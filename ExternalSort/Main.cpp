// ��������� ������������� �����������
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//���������� ������ �������
#define LEN(x)  (sizeof(x) / sizeof((x)[0]))

//��������� ������ ��� ��������� ������
#define TemplateFileName "_sort%03d.dat"
//������������ ������ �����
#define LengthName 13

//���������� �������� ���������
#define cmpLow(x,y) (x < y)
#define cmpGreater(x,y) (x > y)

//���������� �������
#define CountElem 100
// ��������� ������ ��� �������� �����
typedef long int keyType;
//��������� ������ ��� �������� ���������
typedef struct recTypeTag {
    keyType key;                                  // �������� ��������
#if CountElem
    char data[CountElem - sizeof(keyType)];       // ���� ��������
#endif
} recType;

//���������� bool
typedef enum {
    false,
    true
} boolean;

//��������������� ��������� ������, ��������������� ��� �������� ���������� � �����
typedef struct tmpFileTag {
    FILE* FileElem;                 // ����
    char FileName[LengthName];      // ��� �����
    recType rec;                    // ��������� ������, ������� ���� ���������
    int dummy;                      // ���������� �������� ��������
    boolean FlagEndFile;            // ���� ����� �����
    boolean FlagEndRun;             // ���� ����� �������
    boolean valid;                  // ����������� ������
    int Fibonacci;                        // ��������� ����� ���������
} tmpFileType;

static tmpFileType** MassTmpFile;       // ������ ������ � ��������� ������
static size_t CountTMPEemFile;          // ���������� ��������� ������ 
static char* InputFileName;             // ��� �������� ����� 
static char* OutputFileName;            // ��� ��������� ����� 

static int level;                       // ������� ��������
static int NodeCount;                   // ���������� ����� ��� ������ ������ 


    //���������� ����
typedef struct InternalNode {
    struct InternalNode* parent;    //��������
    struct ExternalNode* passed;    //������� ����
} InternalNodeType;

//������� ����
typedef struct ExternalNode {
    struct InternalNode* parent;    //��������
    recType rec;                    //������
    int run;                        //����� �������
    boolean valid;                  //��������� ������
} ExternalNodeType;

//����
typedef struct Node {
    InternalNodeType i;            //���������� ����
    ExternalNodeType e;            //������� ����
} NodeType;

static NodeType* node;             //������ �����
static ExternalNodeType* win;      //��������� ����
static FILE* Selected;             //������� ����
static boolean InputEOF;           //���� ��������� �������� �����
static int maxRun;                 //������������ ���������� ��������
static int curRun;                 //����� �������� �������
InternalNodeType* p;               //���������� ����
static boolean lastKeyValid;       //��������� ���������� ������� �����
static keyType lastKey;            //��������� ������ ���������� �����

// ������� ��� ������������ ��������
void DeleteTmpFiles(void)
{
    if (MassTmpFile) {
        size_t n = LEN(MassTmpFile);     //������ ������� ������
        for (size_t i = 0; i < n; i++) {
            if (MassTmpFile[i]) {
                if (MassTmpFile[i]->FileElem)
                {
                    fclose(MassTmpFile[i]->FileElem);
                }
                if (*MassTmpFile[i]->FileName)
                {
                    remove(MassTmpFile[i]->FileName);
                }
                free(MassTmpFile[i]);
            }
        }
        free(MassTmpFile);
    }
}

// ������� ���������� � ������������ ��������
void TerminateTmpFiles(int rc)
{

    //������� ����� ��� ������ � ���� ����������
    remove(OutputFileName);

    if (rc == 0) {
        //������ �����, ����������� ����������
        size_t fileT = CountTMPEemFile - 1;

        fclose(MassTmpFile[fileT]->FileElem); MassTmpFile[fileT]->FileElem = NULL;

        if (rename(MassTmpFile[fileT]->FileName, OutputFileName)) {
            perror("io1");
            DeleteTmpFiles();
            exit(1);
        }

        *MassTmpFile[fileT]->FileName = 0;
    }
    DeleteTmpFiles();
}

//������� ������ �� ���������
void TermProgram(int rc)
{
    TerminateTmpFiles(rc);
    exit(rc);
}

// ������� ��������� ������
void* AllocatedMemory(size_t size) {
    //����������, ���������� ���������� ��������� ������
    void* p;

    // ��������� �������� ������ � ����������������
    if ((p = calloc(1, size)) == NULL) {
        printf("error: malloc failed, size = %d\n", size);
        TermProgram(1);
    }
    return p;
}

// ������� ������������� ��������� ������ ����������
void InitValueTmpFiles(void) {
    // �������������� ����� ��� �������
    if (CountTMPEemFile < 3)
    {
        CountTMPEemFile = 3;
    }

    MassTmpFile = AllocatedMemory(CountTMPEemFile * sizeof(tmpFileType*));
    //����������, ���������� � ���� ���������� � �����
    tmpFileType* fileInfo = AllocatedMemory(CountTMPEemFile * sizeof(tmpFileType));

    for (int i = 0; i < CountTMPEemFile; i++) {
        MassTmpFile[i] = fileInfo + i;
        //�������� ����� ���������� �����, �������� �������
        sprintf(MassTmpFile[i]->FileName, TemplateFileName, i);

        if ((MassTmpFile[i]->FileElem = fopen(MassTmpFile[i]->FileName, "w+b")) == NULL) {
            perror("io2");
            TermProgram(1);
        }
    }
}

// ������� ������ ������
recType* ReadRecord(void) {

    //�������� �� ������ �����
    if (node == NULL) {

        if (NodeCount < 2)
        {
            NodeCount = 2;
        }

        node = AllocatedMemory(NodeCount * sizeof(NodeType));

        //��������� ��������� ������ ����� ������ � ����������
        for (int i = 0; i < NodeCount; i++) {
            node[i].i.passed = &node[i].e;
            node[i].i.parent = &node[i / 2].i;
            node[i].e.parent = &node[(NodeCount + i) / 2].i;
            node[i].e.run = 0;
            node[i].e.valid = false;
        }

        win = &node[0].e;
        lastKeyValid = false;

        if ((Selected = fopen(InputFileName, "rb")) == NULL) {
            printf("error: file %s, unable to open\n", InputFileName);
            TermProgram(1);
        }
    }

    while (true) {

        //��������� ����������� ���������� ����� �������
        if (!InputEOF) {
            if (fread(&win->rec, sizeof(recType), 1, Selected) == 1) {
                if (
                    (!lastKeyValid ||
                        cmpLow(win->rec.key, lastKey)) &&
                    (++win->run > maxRun))
                {
                    maxRun = win->run;
                }
                win->valid = true;
            }
            else if (feof(Selected)) {
                fclose(Selected);
                InputEOF = true;
                win->valid = false;
                win->run = maxRun + 1;
            }
            else {
                perror("io4");
                TermProgram(1);
            }
        }
        else {
            win->valid = false;
            win->run = maxRun + 1;
        }

        //������������� ��������� ���������� � �� ����������
        p = win->parent;
        do {
            boolean isSwap;
            isSwap = false;
            if (p->passed->run < win->run) {
                isSwap = true;
            }
            else if (p->passed->run == win->run) {
                if (p->passed->valid && win->valid) {
                    if (cmpLow(p->passed->rec.key, win->rec.key))
                    {
                        isSwap = true;
                    }
                }
                else {
                    isSwap = true;
                }
            }
            if (isSwap) {
                //p ������ ����� ������ ����������
                ExternalNodeType* t;

                t = p->passed;
                p->passed = win;
                win = t;
            }
            p = p->parent;
        } while (p != &node[0].i);

        //�������� �� ������� ��������� �������
        if (win->run != curRun) {
            // win->run = curRun + 1
            if (win->run > maxRun) {
                // ����� ������
                free(node);
                return NULL;
            }
            curRun = win->run;
        }

        // ������� ���� ������
        if (win->run) {
            lastKey = win->rec.key;
            lastKeyValid = true;
            return &win->rec;
        }
    }
}

// ������� �������� �������
void CreateRuns(void) {
    recType* win;       //���������

    //���������� ����
    int fileT = CountTMPEemFile - 1;
    //����� ����
    int fileP = CountTMPEemFile - 2;

    for (int j = 0; j < fileT; j++) {
        MassTmpFile[j]->Fibonacci = 1;
        MassTmpFile[j]->dummy = 1;
    }

    MassTmpFile[fileT]->Fibonacci = 0;
    MassTmpFile[fileT]->dummy = 0;

    level = 1;

    win = ReadRecord();
    while (win) {
        boolean anyrun;

        anyrun = false;
        for (int j = 0; win && j <= fileP; j++) {
            boolean run;

            run = false;
            if (MassTmpFile[j]->valid) {
                if (!cmpLow(win->key, MassTmpFile[j]->rec.key)) {
                    //���������� �������� � ������������� �������
                    run = true;
                }
                else if (MassTmpFile[j]->dummy) {
                    //������� ����� ������
                    MassTmpFile[j]->dummy--;
                    run = true;
                }
            }
            else {
                //������ ������ � �����
                MassTmpFile[j]->dummy--;
                run = true;
            }

            if (run) {
                anyrun = true;

                //������ ������
                while (true) {
                    if (fwrite(win, sizeof(recType), 1, MassTmpFile[j]->FileElem) != 1) {
                        perror("io3");
                        TermProgram(1);
                    }
                    MassTmpFile[j]->rec.key = win->key;
                    MassTmpFile[j]->valid = true;

                    if (((win = ReadRecord()) == NULL) ||
                        cmpLow(win->key, MassTmpFile[j]->rec.key))
                    {
                        break;
                    }
                }
            }
        }

        //���� ����������� ����� ��� �������� - ����� �� �������
        if (!anyrun) {
            level++;
            int t = MassTmpFile[0]->Fibonacci;
            for (int j = 0; j <= fileP; j++) {
                MassTmpFile[j]->dummy = t + MassTmpFile[j + 1]->Fibonacci - MassTmpFile[j]->Fibonacci;
                MassTmpFile[j]->Fibonacci = t + MassTmpFile[j + 1]->Fibonacci;
            }
        }
    }
}

// ������� ���������� ��������� ������� ������
void RefreshFlag(int j) {
    //���� � ������ file[j] � ������ ������ ������
    MassTmpFile[j]->FlagEndRun = false;
    MassTmpFile[j]->FlagEndFile = false;
    rewind(MassTmpFile[j]->FileElem);
    if (fread(&MassTmpFile[j]->rec, sizeof(recType), 1, MassTmpFile[j]->FileElem) != 1) {
        if (feof(MassTmpFile[j]->FileElem)) {
            MassTmpFile[j]->FlagEndRun = true;
            MassTmpFile[j]->FlagEndFile = true;
        }
        else {
            perror("io5");
            TermProgram(1);
        }
    }
}

// ������� ����������� ���������� ��������
void MultiphaseMergeSort(void) {
    int fileT = CountTMPEemFile - 1;
    int fileP = CountTMPEemFile - 2;

    //��������� ������ �����������
    for (int j = 0; j < fileT; j++)
    {
        RefreshFlag(j);
    }

    //������� ��������
    while (level)
    {
        while (true)
        {
            //�������� �� ������� ��������
            boolean allDummies = true;
            boolean anyRuns = false;
            for (int j = 0; j <= fileP; j++) {
                if (!MassTmpFile[j]->dummy) {
                    allDummies = false;
                    if (!MassTmpFile[j]->FlagEndFile) anyRuns = true;
                }
            }

            if (anyRuns) {

                //������� ���� ������
                while (true) {
                    int k = -1;
                    for (int j = 0; j <= fileP; j++) {
                        if (MassTmpFile[j]->FlagEndRun) continue;
                        if (MassTmpFile[j]->dummy) continue;
                        if (k < 0 ||
                            (k != j &&
                                cmpGreater(MassTmpFile[k]->rec.key, MassTmpFile[j]->rec.key)))
                        {
                            k = j;
                        }
                    }
                    if (k < 0)
                    {
                        break;
                    }

                    //��������� ������ � file[fileT]
                    if (fwrite(&MassTmpFile[k]->rec, sizeof(recType), 1,
                        MassTmpFile[fileT]->FileElem) != 1) {
                        perror("io6");
                        TermProgram(1);
                    }

                    //�������� ������
                    keyType lastKey = MassTmpFile[k]->rec.key;
                    if (fread(&MassTmpFile[k]->rec, sizeof(recType), 1,
                        MassTmpFile[k]->FileElem) == 1) {
                        //�������� �� ������ �������
                        if (cmpLow(MassTmpFile[k]->rec.key, lastKey))
                        {
                            MassTmpFile[k]->FlagEndRun = true;
                        }
                    }
                    else if (feof(MassTmpFile[k]->FileElem)) {
                        MassTmpFile[k]->FlagEndFile = true;
                        MassTmpFile[k]->FlagEndRun = true;
                    }
                    else {
                        perror("io7");
                        TermProgram(1);
                    }
                }

                //���������, ��������� � ��������� ���������
                for (int j = 0; j <= fileP; j++) {
                    if (MassTmpFile[j]->dummy)
                    {
                        MassTmpFile[j]->dummy--;
                    }
                    if (!MassTmpFile[j]->FlagEndFile)
                    {
                        MassTmpFile[j]->FlagEndRun = false;
                    }
                }

            }
            else if (allDummies) {
                for (int j = 0; j <= fileP; j++)
                {
                    MassTmpFile[j]->dummy--;
                }
                MassTmpFile[fileT]->dummy++;
            }

            //����� �������
            if (MassTmpFile[fileP]->FlagEndFile && !MassTmpFile[fileP]->dummy)
            {
                level--;
                if (!level)
                {
                    return;
                }

                fclose(MassTmpFile[fileP]->FileElem);
                if ((MassTmpFile[fileP]->FileElem = fopen(MassTmpFile[fileP]->FileName, "w+b"))
                    == NULL)
                {
                    perror("io8");
                    TermProgram(1);
                }
                MassTmpFile[fileP]->FlagEndFile = false;
                MassTmpFile[fileP]->FlagEndRun = false;

                RefreshFlag(fileT);

                tmpFileType* TempFile = MassTmpFile[fileT];
                memmove(MassTmpFile + 1, MassTmpFile, fileT * sizeof(tmpFileType*));
                MassTmpFile[0] = TempFile;

                //������ ����� �������
                for (int j = 0; j <= fileP; j++)
                {
                    if (!MassTmpFile[j]->FlagEndFile)
                    {
                        MassTmpFile[j]->FlagEndRun = false;
                    }
                }
            }
        }
    }
}

// ������� ������������� ��������� �������� ���������
void InitProgram(void)
{
    InputFileName = "input.txt";
    OutputFileName = "output.txt";
    CountTMPEemFile = 5;
    NodeCount = 2000;
}

// �������, ������������ ��������� ������
void MakeRandom(recType* data, long N) {
    for (long int i = 0; i < N; i++)
    {
        data[i].key = rand() | (rand() << 16);
    }
}

// �������, ��������� ���� � ���������� �������
void CreateRandomInput(void)
{
    recType d[CountElem];

    MakeRandom(d, CountElem);

    FILE* f = fopen(InputFileName, "r+b");
    for (int i = 0; i < CountElem; i++)
    {
        fwrite(&d[i], sizeof(recType), 1, f);
    }
    fclose(f);
}

//�������, ��������� � ������� ��������� ������ ���������
void CreateOutput(void)
{
    recType d[CountElem];

    FILE* f = fopen(OutputFileName, "r+b");
    for (int i = 0; i < CountElem; i++)
    {
        fread(&d[i], sizeof(recType), 1, f);
        printf("%ld\n", d[i].key);
    }
    fclose(f);
}

// ������� ����������� ������ ���������
void RunSort(void) {
    InitProgram();
    CreateRandomInput();
    InitValueTmpFiles();
    CreateRuns();
    MultiphaseMergeSort();
    CreateOutput();
    TerminateTmpFiles(0);
}

int main(int argc, char* argv[]) {

    RunSort();

    return 0;
}