// Директива игнорирования устаревания
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Вычисление длинны массива
#define LEN(x)  (sizeof(x) / sizeof((x)[0]))

//Шаблонные данные для временных файлов
#define TemplateFileName "_sort%03d.dat"
//Максимальная длинна имени
#define LengthName 13

//Объявление оператор сравнения
#define cmpLow(x,y) (x < y)
#define cmpGreater(x,y) (x > y)

//Количество записей
#define CountElem 100
// Структуры данных для хранения ключа
typedef long int keyType;
//Структура данных для хранения элементов
typedef struct recTypeTag {
    keyType key;                                  // Ключевое значение
#if CountElem
    char data[CountElem - sizeof(keyType)];       // Поле значений
#endif
} recType;

//Реализация bool
typedef enum {
    false,
    true
} boolean;

//Вспомогательная структура данных, предназначенная для хранения информации о файле
typedef struct tmpFileTag {
    FILE* FileElem;                 // Файл
    char FileName[LengthName];      // Имя файла
    recType rec;                    // Последняя запись, которая была прочитана
    int dummy;                      // Количество холостых проходов
    boolean FlagEndFile;            // Флаг конца файла
    boolean FlagEndRun;             // Флаг конца прохода
    boolean valid;                  // Пригодность записи
    int Fibonacci;                        // Идеальное число Фибоначчи
} tmpFileType;

static tmpFileType** MassTmpFile;       // Массив данных о временных файлах
static size_t CountTMPEemFile;          // Количество временных файлов 
static char* InputFileName;             // Имя входного файла 
static char* OutputFileName;            // Имя выходного файла 

static int level;                       // уровень проходов
static int NodeCount;                   // количество узлов для дерева выбора 


    //внутренний узел
typedef struct InternalNode {
    struct InternalNode* parent;    //Родитель
    struct ExternalNode* passed;    //Внешний узел
} InternalNodeType;

//внешний узел
typedef struct ExternalNode {
    struct InternalNode* parent;    //Родитель
    recType rec;                    //Запись
    int run;                        //Номер пробега
    boolean valid;                  //Состояние записи
} ExternalNodeType;

//Узел
typedef struct Node {
    InternalNodeType i;            //внутренний узел
    ExternalNodeType e;            //внешний узел
} NodeType;

static NodeType* node;             //Массив узлов
static ExternalNodeType* win;      //Выбранный узел
static FILE* Selected;             //Входной файл
static boolean InputEOF;           //Флаг окончания входного файла
static int maxRun;                 //Максимальное количество пробегов
static int curRun;                 //Номер текущего прохода
InternalNodeType* p;               //Внутренние узлы
static boolean lastKeyValid;       //Состояние последнего взятого ключа
static keyType lastKey;            //Состояние записи последнего ключа

// Функция для освобождения ресурсов
void DeleteTmpFiles(void)
{
    if (MassTmpFile) {
        size_t n = LEN(MassTmpFile);     //Длинна массива файлов
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

// Функция подготовки к освобождению ресурсов
void TerminateTmpFiles(int rc)
{

    //Очистка файла для вывода в него результата
    remove(OutputFileName);

    if (rc == 0) {
        //Индекс файла, содержащего результаты
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

//Функция выхода из программы
void TermProgram(int rc)
{
    TerminateTmpFiles(rc);
    exit(rc);
}

// Функция выделения памяти
void* AllocatedMemory(size_t size) {
    //Переменная, содержащяя выделенную свободную память
    void* p;

    // Безопасно выделить память и инициализоваться
    if ((p = calloc(1, size)) == NULL) {
        printf("error: malloc failed, size = %d\n", size);
        TermProgram(1);
    }
    return p;
}

// Функция инициализации временных файлов значениями
void InitValueTmpFiles(void) {
    // инициализовать файлы для слияния
    if (CountTMPEemFile < 3)
    {
        CountTMPEemFile = 3;
    }

    MassTmpFile = AllocatedMemory(CountTMPEemFile * sizeof(tmpFileType*));
    //Переменная, содержащая в себе информацию о файле
    tmpFileType* fileInfo = AllocatedMemory(CountTMPEemFile * sizeof(tmpFileType));

    for (int i = 0; i < CountTMPEemFile; i++) {
        MassTmpFile[i] = fileInfo + i;
        //Создание имени временного файла, согласно шаблону
        sprintf(MassTmpFile[i]->FileName, TemplateFileName, i);

        if ((MassTmpFile[i]->FileElem = fopen(MassTmpFile[i]->FileName, "w+b")) == NULL) {
            perror("io2");
            TermProgram(1);
        }
    }
}

// Функция чтения записи
recType* ReadRecord(void) {

    //Проверка на первый выход
    if (node == NULL) {

        if (NodeCount < 2)
        {
            NodeCount = 2;
        }

        node = AllocatedMemory(NodeCount * sizeof(NodeType));

        //Прочитать следующую запись путем выбора с замещением
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

        //заместить предыдущего победителя новой записью
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

        //Отсортировать Родителей выбранного и не выбранного
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
                //p должно иметь статус выбранного
                ExternalNodeType* t;

                t = p->passed;
                p->passed = win;
                win = t;
            }
            p = p->parent;
        } while (p != &node[0].i);

        //Проверка на условие окончания прохода
        if (win->run != curRun) {
            // win->run = curRun + 1
            if (win->run > maxRun) {
                // конец вывода
                free(node);
                return NULL;
            }
            curRun = win->run;
        }

        // вывести верх дерева
        if (win->run) {
            lastKey = win->rec.key;
            lastKeyValid = true;
            return &win->rec;
        }
    }
}

// Функция создания прохода
void CreateRuns(void) {
    recType* win;       //Выбранный

    //Предыдущий файл
    int fileT = CountTMPEemFile - 1;
    //Новый файл
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
                    //Необходимо добавить к существующему пробегу
                    run = true;
                }
                else if (MassTmpFile[j]->dummy) {
                    //Создать новый проход
                    MassTmpFile[j]->dummy--;
                    run = true;
                }
            }
            else {
                //первый проход в файле
                MassTmpFile[j]->dummy--;
                run = true;
            }

            if (run) {
                anyrun = true;

                //полный проход
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

        //Если закончилось место для проходов - вверх на уровень
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

// Функция обновления состояний массива файлов
void RefreshFlag(int j) {
    //Идти в начало file[j] и читать первую запись
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

// Функция организации сортировки слиянием
void MultiphaseMergeSort(void) {
    int fileT = CountTMPEemFile - 1;
    int fileP = CountTMPEemFile - 2;

    //Заполняем массив информацией
    for (int j = 0; j < fileT; j++)
    {
        RefreshFlag(j);
    }

    //Слияние проходов
    while (level)
    {
        while (true)
        {
            //Проверка на предмет проходов
            boolean allDummies = true;
            boolean anyRuns = false;
            for (int j = 0; j <= fileP; j++) {
                if (!MassTmpFile[j]->dummy) {
                    allDummies = false;
                    if (!MassTmpFile[j]->FlagEndFile) anyRuns = true;
                }
            }

            if (anyRuns) {

                //Сливаем один проход
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

                    //Занесение записи в file[fileT]
                    if (fwrite(&MassTmpFile[k]->rec, sizeof(recType), 1,
                        MassTmpFile[fileT]->FileElem) != 1) {
                        perror("io6");
                        TermProgram(1);
                    }

                    //Заменить запись
                    keyType lastKey = MassTmpFile[k]->rec.key;
                    if (fread(&MassTmpFile[k]->rec, sizeof(recType), 1,
                        MassTmpFile[k]->FileElem) == 1) {
                        //Проверка на концец пробега
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

                //Изменения, связанные с холостыми проходами
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

            //конец прохода
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

                //начать новые проходы
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

// Функция инициализации начальных значений программы
void InitProgram(void)
{
    InputFileName = "input.txt";
    OutputFileName = "output.txt";
    CountTMPEemFile = 5;
    NodeCount = 2000;
}

// Функция, генерирующая случайные данные
void MakeRandom(recType* data, long N) {
    for (long int i = 0; i < N; i++)
    {
        data[i].key = rand() | (rand() << 16);
    }
}

// Функция, создающая файл с случайными числами
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

//Функция, выводящая в консоль результат работы алгоритма
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

// Функция организации работы программы
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