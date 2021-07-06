/*
*   CSE - System Programming - Midterm Project
*   Gokhan Has - 161044067
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h> 
#include <string.h>


#define FALSE 0
#define TRUE 1

#define SOUP 10
#define MAIN_COURSE 11
#define DESERT 12

// This global variable will be used to write to STDOUT_FILENO by write syscall.
char print[700];

// Function define ...

/*
 * This function prints errors with perror and ends the program with exit gracefully.
 */
void errorExit(char* error);

/*
 * This function prints how the program is used with write syscall.
 */
void printUsage();

/*
 * This function prints the constraints of the program with write syscall.
 */
void printConstraints();

/*
 *  This function checks in the pid_t array to check if all processes are finished. If even one process id equals -1, 
 * the process is not over. If all are different from -1, all processes are finished.
 * 
 *  arr --> is array which keeps the process ids 
 *  size --> is pid_t arr size
 */ 
int isFinal(pid_t arr[], int size);

/*
 *  This function returns what index a given process id is in pid_t arr. If not, it returns -1.
 * 
 * arr --> is array which keeps the process ids
 * processID --> is pid_t which keeps the process id
 * size --> is pid_t arr size
 */ 
int getProcessIndex(pid_t arr[], pid_t processID, int size);

/*
 * This function checks whether the given process id is among the given ranges. The getProcessIndex () function is used inside.
 * 
 * arr --> is array which keeps the process ids
 * processID --> is pid_t which keeps the process id
 * size --> is pid_t arr size
 * range1 --> is int, keeps the small range
 * range2 --> is int, keeps the big range
 */ 
int ifIsRange(pid_t arr[], pid_t processID, int size, int range1, int range2);

/*
 *  The Supplier function takes many parameters. It takes the plate one by one to the kitchen 
 * by reading from the file given with the fd parameter it takes.
 * 
 * sem_P --> number of soups in the kitchen
 * sem_C --> number of main courses in the kitchen
 * sem_D --> number of deserts in the kitchen
 * sizeOfKitchen --> The number of free spaces in the kitchen is initially 2 * L * M + 1.
 * mutexKitchen --> It will be used to prevent both supplier and cook from entering the kitchen at the same time.
 * size --> (L*M)
 * fd --> The file descriptor is required for the file that the supplier will read.
 */ 
int theSupplier(sem_t* sem_P, sem_t* sem_C, sem_t* sem_D, sem_t* sizeOfKitchen, sem_t* mutexKitchen, int size, int fd);

/*
 * sem_P_fromSupplier --> The number of soups in the kitchen
 * sem_C_fromSupplier --> The number of main courses in the kitchen
 * sem_D_fromSupplier --> The number of deserts in the kitchen
 * 
 * sem_P_toCounter --> Total number of soups taken to the counter
 * sem_C_toCounter --> Total number of main courses taken to the counter
 * sem_D_toCounter --> Total number of deserts taken to the counter
 * 
 * sizeofKitchen --> The number of free spaces in the kitchen is initially 2 * L * M + 1.
 * mutexKitchen --> It will be used to prevent both supplier and cook from entering the kitchen at the same time.
 * 
 * mutexCook --> Multiple cooks should be prevented from entering the kitchen at the same time.
 * mutexCounter --> Leaving food on the counter or taking it at the same time should be prevented.
 * counterSize --> The number of free plates in the counter
 * 
 * counterSoupSize --> At that moment, the number of soups on the counter
 * counterMainCourseSize --> At that moment, the number of main courses on the counter
 * counterDesertSize --> At that moment, the number of deserts on the counter
 * 
 * size --> L * M
 * N --> Total number of cooks
 */  
int theCook(sem_t* sem_P_fromSupplier, sem_t* sem_C_fromSupplier, sem_t* sem_D_fromSupplier, sem_t* sem_P_toCounter, sem_t* sem_C_toCounter, sem_t* sem_D_toCounter, 
            sem_t* sizeofKitchen, sem_t* mutexKitchen, sem_t* mutexCook, sem_t* mutexCounter, sem_t* counterSize, sem_t* counterSoupSize, sem_t* counterMainCourseSize, sem_t* counterDesertSize, int size, int N);


/*
 * counterSoupSize --> At that moment, the number of soups on the counter
 * counterMainCourseSize --> At that moment, the number of main courses on the counter
 * counterDesertSize --> At that moment, the number of deserts on the counter
 * 
 * counterSize --> The number of free plates in the counter
 * mutexCounter --> Leaving food on the counter or taking it at the same time should be prevented.
 * emptyTable --> The number of empty tables is kept
 * mutexGraduated --> Graduated students will eat first
 * limit --> L parameter
 * G or U --> Number of students graduating or normal students
 */ 
int theGraduated(sem_t* counterSoupSize, sem_t* counterMainCourseSize, sem_t* counterDesertSize, sem_t* counterSize, sem_t* mutexCounter, sem_t* mutexStudentsGraduated, sem_t* emptyTable, sem_t* mutexGraduated, sem_t* totalStudents, int limit, int G);
int theStudent(sem_t* counterSoupSize, sem_t* counterMainCourseSize, sem_t* counterDesertSize, sem_t* counterSize, sem_t* mutexCounter, sem_t* mutexStudents, sem_t* emptyTable, sem_t* mutexGraduated, sem_t* totalStudents, int limit, int U);

/*
 * Returns TRUE if semaphore's value is equal to 0, or FALSE if not. It is written to check if there is free space in the kitchen.
 */ 
int ifKitchenHasNoSize(sem_t* semp);

/*
 * Adds and returns the value of 3 semaphore flour given as parameters.
 */ 
int totalItems(sem_t* sem_P, sem_t* sem_C, sem_t* sem_D);

/*
 *  It takes the semaphores that hold the number of meals on the counter as parameters and 
 * returns that food, whichever is less. Thus, it is provided to prevent deadlock.
 */ 
int returnLeastObject(sem_t* sem_P, sem_t* sem_C, sem_t* sem_D); 

/*
 * Returns the value of semaphore that is sent as a parameter.
 */ 
int getValueFromSem(sem_t* sem);

/*
 *  This function takes fd as a parameter and reads a byte file. Returns SOUP if 'p' or 'P', 
 * MAIN_COURSE if 'c' or 'C', and finally DESERT if 'd' or 'D'.
 */ 
int getRandomPlate(int fd);

/*
 * Returns the total number of bytes of the file name given as a parameter.
 */ 
size_t getFileSize(const char* fileName);

/*
 * It is written to capture the signal. Signal no 2 is the ctrl + c signal (SIGINT).
 */ 
void catcher(int signalNo) {
    if(signalNo == 2) {
        sprintf(print,"SIGINT is caught \n");
        write(STDOUT_FILENO,print,strlen(print));
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char **argv) {
    
    pid_t mainProcess = getpid();
    signal(SIGINT,catcher);

    char *printMessage = NULL;
    char *nValue = NULL;
    char *tValue = NULL;
    char *sValue = NULL;
    char *lValue = NULL; 
    char *uValue = NULL;
    char *gValue = NULL;
    char *fValue = NULL;

    int N = 0, M = 0, T = 0, S = 0, L = 0, U = 0, G = 0, kitchenSize = 0;

    int nIndex = 0;
    int tIndex = 0;
    int sIndex = 0;
    int lIndex = 0;
    int uIndex = 0;
    int gIndex = 0;
    int fIndex = 0;

    int another = 0;
    int c = 0;

    while ((c = getopt(argc, argv, "N:T:S:L:U:G:F:")) != -1)
    switch (c) {
        case 'N':
            nIndex += 1;
            nValue = optarg;
            break;
        
        case 'T':
            tIndex += 1;
            tValue = optarg;
            break;
        
        case 'S':
            sIndex += 1;
            sValue = optarg;
            break;

        case 'L':
            lIndex += 1;
            lValue = optarg;
            break;

        case 'U':
            uIndex += 1;
            uValue = optarg;
            break;
        
        case 'G':
            gIndex += 1;
            gValue = optarg;
            break;

        case 'F':
            fIndex += 1;
            fValue = optarg;
            break;

        case '?':
            another += 1;
            break;

        default:
            abort();
    }

    if(nIndex == 1 && tIndex == 1 && sIndex == 1 && lIndex == 1 && uIndex == 1 && gIndex == 1 && fIndex == 1 && another == 0) {
        N = atoi(nValue);
        T = atoi(tValue);
        S = atoi(sValue);
        L = atoi(lValue);
        U = atoi(uValue);
        G = atoi(gValue);

        M =  G + U;

        if(!(U > G >= 1)) {
            errno = EIO;
            printUsage();
            printConstraints();
            errorExit("ERROR -U or -G ");
        }
        
        if(!(M > N && N > 2)) {
            errno = EIO;
            printUsage();
            printConstraints();
            errorExit("ERROR -M or -N ");
        }

        if(!(S > 3)) {
            errno = EIO;
            printUsage();
            printConstraints();
            errorExit("ERROR -S ");
        }

        if(!(M > T && T >= 1)){
            errno = EIO;
            printUsage();
            printConstraints();
            errorExit("ERROR -M or -T ");
        }

        if(!(L >= 3)) {
            errno = EIO;
            printUsage();
            printConstraints();
            errorExit("ERROR -L ");
        }

        kitchenSize = 2 * L * M + 1; 
             
    }
    else {
        errno = EIO;
        printUsage();
        printConstraints();
        errorExit("ERROR ");
    }

    // Opening file ...
    int fd = open(fValue, O_RDONLY);
    if (fd == -1) {
        perror("ERROR ! Program filePath (-F) ");
        exit(EXIT_FAILURE);
    }
    
    // It is checked if there are 3 * L * M characters in the file.
    if(getFileSize(fValue) < 3 * L * M) {
        sprintf(print, "ERROR : File size is to small !!!\n");
        write(STDOUT_FILENO,print,strlen(print));
        exit(EXIT_FAILURE);
    }
   
    int pidArr_Size = N + M + 1;
    pid_t pidArr[pidArr_Size];
    
    // Semaphores ...
    sem_t *sp_P_forSupplier = mmap(NULL, sizeof(*sp_P_forSupplier), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_C_forSupplier = mmap(NULL, sizeof(*sp_C_forSupplier), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_D_forSupplier = mmap(NULL, sizeof(*sp_D_forSupplier), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    
    sem_t *sp_sizeOfKitchen = mmap(NULL, sizeof(*sp_sizeOfKitchen), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    
    sem_t *sizeOfCounter = mmap(NULL, sizeof(*sizeOfCounter), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_P_forCook = mmap(NULL, sizeof(*sp_P_forCook), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_C_forCook = mmap(NULL, sizeof(*sp_C_forCook), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_D_forCook = mmap(NULL, sizeof(*sp_D_forCook), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);

    sem_t *sp_emptyOfCounter = mmap(NULL, sizeof(*sp_emptyOfCounter), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);

    sem_t *sp_P_counterSize= mmap(NULL, sizeof(*sp_P_counterSize), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_C_counterSize = mmap(NULL, sizeof(*sp_C_counterSize), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *sp_D_counterSize = mmap(NULL, sizeof(*sp_D_counterSize), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);

    sem_t *sp_emptyTable = mmap(NULL, sizeof(*sp_emptyTable), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);

    sem_t *sp_totalStudents = mmap(NULL, sizeof(*sp_totalStudents), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);

    // Mutexes ...
    sem_t *mutex_kitchen = mmap(NULL, sizeof(*mutex_kitchen), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *mutex_cook = mmap(NULL, sizeof(*mutex_cook), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *mutex_counter = mmap(NULL, sizeof(*mutex_counter), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *mutex_student = mmap(NULL, sizeof(*mutex_student), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);

    sem_t *mutex_graduated = mmap(NULL, sizeof(*mutex_graduated), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);
    sem_t *mutexStudentsGraduated = mmap(NULL, sizeof(*mutexStudentsGraduated), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, 1, 0);


    if ((void *) sp_P_forSupplier == MAP_FAILED) {
        errorExit("ERROR sp_P_forSupplier ");
    }

    if ((void *) sp_C_forSupplier == MAP_FAILED) {
        errorExit("ERROR sp_P_forSupplier ");
    }

    if ((void *) sp_D_forSupplier == MAP_FAILED) {
        errorExit("ERROR sp_P_forSupplier ");
    }

    if ((void *) sp_sizeOfKitchen == MAP_FAILED) {
        errorExit("ERROR sp_sizeOfKitchen");
    }

    if ((void *) mutex_kitchen == MAP_FAILED) {
        errorExit("ERROR mutex_kitchen");
    }

    if ((void *) sizeOfCounter == MAP_FAILED) {
        errorExit("ERROR sizeOfCounter");
    }

    if ((void *) sp_P_forCook == MAP_FAILED) {
        errorExit("ERROR sp_P_forCook");
    }
    
    if ((void *) sp_C_forCook == MAP_FAILED) {
        errorExit("ERROR sp_C_forCook");
    }
    
    if ((void *) sp_D_forCook == MAP_FAILED) {
        errorExit("ERROR sp_D_forCook");
    }
    
    if ((void *) mutex_cook == MAP_FAILED) {
        errorExit("ERROR mutex_cook");
    }

    if ((void *) mutex_counter == MAP_FAILED) {
        errorExit("ERROR mutex_counter");
    }

    if ((void *) sp_emptyOfCounter == MAP_FAILED) {
        errorExit("ERROR sp_emptyOfCounter");
    }

    if ((void *) sp_P_counterSize == MAP_FAILED) {
        errorExit("ERROR sp_P_counterSize");
    }

    if ((void *) sp_C_counterSize == MAP_FAILED) {
        errorExit("ERROR sp_C_counterSize");
    }

    if ((void *) sp_D_counterSize == MAP_FAILED) {
        errorExit("ERROR sp_D_counterSize");
    }
    
    if ((void *) mutex_student == MAP_FAILED) {
        errorExit("ERROR mutex_student");
    }

    if ((void *) sp_emptyTable == MAP_FAILED) {
        errorExit("ERROR sp_emptyTable");
    }
    
    if ((void *) sp_totalStudents == MAP_FAILED) {
        errorExit("ERROR sp_totalStudents");
    }
    
    if ((void *) mutex_graduated == MAP_FAILED) {
        errorExit("ERROR mutex_graduated");
    }

    if ((void *) mutexStudentsGraduated == MAP_FAILED) {
        errorExit("ERROR mutexStudentsGraduated");
    }
    
    // End MAP_FAILED controls ...
    
    if(sem_init(sp_P_forSupplier, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_P_forSupplier ");
    }

    if(sem_init(sp_C_forSupplier, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_C_forSupplier ");
    }

    if(sem_init(sp_D_forSupplier, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_D_forSupplier ");
    }

    if(sem_init(sp_sizeOfKitchen, 1, kitchenSize) == -1) {
        errorExit("ERROR sem_init sp_sizeOfKitchen ");
    }

    if(sem_init(mutex_kitchen, 1, 1) == -1) {
        errorExit("ERROR sem_init mutex_kitchen ");
    }

    if(sem_init(sizeOfCounter, 1, S) == -1) {
        errorExit("ERROR sem_init sizeOfCounter ");
    }

    if(sem_init(sp_P_forCook, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_P_forCook ");
    }
    
    if(sem_init(sp_C_forCook, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_C_forCook ");
    }
    
    if(sem_init(sp_D_forCook, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_D_forCook ");
    }

    if(sem_init(mutex_cook, 1, 1) == -1) {
        errorExit("ERROR sem_init mutex_cook ");
    }

    if(sem_init(mutex_counter, 1, 1) == -1) {
        errorExit("ERROR sem_init mutex_counter ");
    }

    if(sem_init(sp_emptyOfCounter, 1, S) == -1) {
        errorExit("ERROR sem_init sp_emptyOfCounter ");
    }

    if(sem_init(sp_P_counterSize, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_P_counterSize ");
    }

    if(sem_init(sp_C_counterSize, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_C_counterSize ");
    }

    if(sem_init(sp_D_counterSize, 1, 0) == -1) {
        errorExit("ERROR sem_init sp_D_counterSize ");
    }
    
    if(sem_init(mutex_student, 1, 1) == -1) {
        errorExit("ERROR sem_init mutex_student ");
    }

    if(sem_init(sp_emptyTable, 1, T) == -1) {
        errorExit("ERROR sem_init sp_emptyTable ");
    }

    if(sem_init(sp_totalStudents, 1, M) == -1) {
        errorExit("ERROR sem_init sp_totalStudents ");
    }

    if(sem_init(mutex_graduated, 1, 1) == -1) {
        errorExit("ERROR sem_init mutex_graduated ");
    }

    if(sem_init(mutexStudentsGraduated, 1, 1) == -1) {
        errorExit("ERROR sem_init mutexStudentsGraduated ");
    }

    // End sem_init funtions ...
    
    
    // create N + M + 1 process ...
    for(int i=0; i < pidArr_Size; i++)  { 
        if(getpid() == mainProcess) {
            if(fork() == 0)
                pidArr[i] = getpid();
        }
    }
    
    if(getProcessIndex(pidArr, getpid(), pidArr_Size) == 0) {
        // SUPPLIER PROCESS ...
        theSupplier(sp_P_forSupplier,sp_C_forSupplier,sp_D_forSupplier, sp_sizeOfKitchen, mutex_kitchen, (L*M), fd);
    }

    else if(ifIsRange(pidArr,getpid(),pidArr_Size,1 , N + 1) == TRUE) {
        // COOK PROCESS ...
        theCook(sp_P_forSupplier, sp_C_forSupplier, sp_D_forSupplier, sp_P_forCook, sp_C_forCook, sp_D_forCook, sp_sizeOfKitchen, mutex_kitchen, 
                mutex_cook, mutex_counter, sp_emptyOfCounter, sp_P_counterSize, sp_C_counterSize, sp_D_counterSize,(L * M), N);
    }    

    else if(ifIsRange(pidArr, getpid(), pidArr_Size, N + 1, N + G + 1) == TRUE) {
        // STUDENT FOR GRADUATED PROCESS ...
        theGraduated(sp_P_counterSize, sp_C_counterSize, sp_D_counterSize, sp_emptyOfCounter, mutex_counter, mutexStudentsGraduated, sp_emptyTable, mutex_graduated, sp_totalStudents, L, G);
    }

    else if(ifIsRange(pidArr, getpid(), pidArr_Size, N + G + 1, pidArr_Size) == TRUE) {
        // STUDENT PROCESS ...
        theStudent(sp_P_counterSize, sp_C_counterSize, sp_D_counterSize, sp_emptyOfCounter, mutex_counter, mutex_student, sp_emptyTable, mutex_graduated, sp_totalStudents, L, U);
    }

    
    // Waits all child process ...
    for(int i=0; i < pidArr_Size; i++)
        pidArr[i] = wait(NULL); 


    
    if(isFinal(pidArr,pidArr_Size) == TRUE) {
        
        if(sem_destroy(sp_P_forSupplier) == -1) {
            errorExit("ERROR sem_destroy sp_P_forSupplier ");
        }

        if(sem_destroy(sp_C_forSupplier) == -1) {
            errorExit("ERROR sem_destroy sp_C_forSupplier ");
        }

        if(sem_destroy(sp_D_forSupplier) == -1) {
            errorExit("ERROR sem_destroy sp_D_forSupplier ");
        }

        if(sem_destroy(sp_sizeOfKitchen) == -1) {
            errorExit("ERROR sem_destroy sp_sizeOfKitchen ");
        }

        if(sem_destroy(mutex_kitchen) == -1) {
            errorExit("ERROR sem_destroy mutex_kitchen ");
        }

        if(sem_destroy(sizeOfCounter) == -1) {
            errorExit("ERROR sem_destroy sizeOfCounter ");
        }

        if(sem_destroy(sp_P_forCook) == -1) {
            errorExit("ERROR sem_destroy sp_P_forCook ");
        }
        
        if(sem_destroy(sp_C_forCook) == -1) {
            errorExit("ERROR sem_destroy sp_C_forCook ");
        }
        
        if(sem_destroy(sp_D_forCook) == -1) {
            errorExit("ERROR sem_destroy sp_D_forCook ");
        }

        if(sem_destroy(mutex_cook) == -1) {
            errorExit("ERROR sem_destroy mutex_cook ");
        }

        if(sem_destroy(mutex_counter) == -1) {
            errorExit("ERROR sem_destroy mutex_counter ");
        }

        if(sem_destroy(sp_emptyOfCounter) == -1) {
            errorExit("ERROR sem_destroy sp_emptyOfCounter ");
        }
        
        if(sem_destroy(sp_P_counterSize) == -1) {
            errorExit("ERROR sem_destroy sp_P_counterSize ");
        }

        if(sem_destroy(sp_C_counterSize) == -1) {
            errorExit("ERROR sem_destroy sp_C_counterSize ");
        }

        if(sem_destroy(sp_D_counterSize) == -1) {
            errorExit("ERROR sem_destroy sp_D_counterSize ");
        }

        if(sem_destroy(mutex_student) == -1) {
            errorExit("ERROR sem_destroy mutex_student ");
        }

        if(sem_destroy(sp_emptyTable) == -1) {
            errorExit("ERROR sem_destroy sp_emptyTable ");
        }

        if(sem_destroy(sp_totalStudents) == -1) {
            errorExit("ERROR sem_destroy sp_totalStudents ");
        }

        if(sem_destroy(mutex_graduated) == -1) {
            errorExit("ERROR sem_destroy mutex_graduated ");
        }

        if(sem_destroy(mutexStudentsGraduated) == -1) {
            errorExit("ERROR sem_destroy mutexStudentsGraduated ");
        }

    }
}



void errorExit(char* error) {
    perror(error);
    exit(EXIT_FAILURE);
}

void printUsage() {
    char* print = NULL; 
    print = "\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "################## USAGE ####################\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        -N : number of cooks               #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        -T : number of tables              #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        -S : a counter of size             #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        -L : a total of L times            #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#  -U : number of undergraduated students   #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#    -G : number of graduated students      #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#    -F : filename for consist of p,c,d     #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#############################################\n\n";
    write(STDOUT_FILENO,print,strlen(print));
    
    print = NULL;
}

void printConstraints() {
    char* print = NULL; 
    print = "\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "####### CONSTRAINTS #######\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        M = U + G        #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#       U > G >= 1        #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        M > N > 2        #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#          S > 3          #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#        M > T >= 1       #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#         L >= 3          #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "#      K = 3(L-1)M + 1    #\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = "###########################\n\n";
    write(STDOUT_FILENO,print,strlen(print));
    print = NULL;
}

int isFinal(pid_t arr[], int size) {

    for(int i = 0; i < size; i++) {
        if(arr[i] == -1 ) {
            return FALSE;
        }
    }
    return TRUE;
}

int getProcessIndex(pid_t arr[], pid_t processID, int size) {
    int index = -1;
    for(int i = 0; i < size; i++) {
        if(processID == arr[i]) {
            index = i;    
        }
    }
    return index;
}

int ifIsRange(pid_t arr[], pid_t processID, int size, int range1, int range2) {
    int index = getProcessIndex(arr, processID, size);
    if(range1 <= index && range2 > index)
        return TRUE;
    return FALSE;
}

int theSupplier(sem_t* sem_P, sem_t* sem_C, sem_t* sem_D, sem_t* sizeOfKitchen, sem_t* mutexKitchen, int size, int fd) {
    
    int soupIndex = 0, mainCourseIndex = 0, desertIndex = 0; 
    lseek(fd,0,SEEK_SET);
    while((soupIndex + mainCourseIndex + desertIndex) < 3*size) {
        
        if(ifKitchenHasNoSize(sizeOfKitchen) == TRUE) {
            sprintf(print,"The supplier is waiting for kitchen room to free - # of total items at kitchen : %d\n",totalItems(sem_P,sem_C,sem_D));
            write(STDOUT_FILENO,print,strlen(print));
        }
        
        switch (getRandomPlate(fd)) {
            
            case SOUP:
            {
                if(soupIndex >= size)
                    break;
                sem_wait(sizeOfKitchen);
                sem_wait(mutexKitchen); // mutfakta kimse var mi ...
                
                sprintf(print, "The supplier supplier is going to the kitchen to deliver soup:  kitchen items P: %d, C: %d, D: %d  = %d\n",getValueFromSem(sem_P), getValueFromSem(sem_C), getValueFromSem(sem_D),totalItems(sem_P,sem_C,sem_D));
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(sem_P);
                soupIndex++;
                sprintf(print, "The supplier delivered soup - after delivery:  kitchen items P: %d, C: %d, D: %d  = %d\n",getValueFromSem(sem_P), getValueFromSem(sem_C), getValueFromSem(sem_D),totalItems(sem_P,sem_C,sem_D));
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(mutexKitchen); // Mutfaga baska biri girebilir ...
                break;
            }
            case MAIN_COURSE:
            {
                if(mainCourseIndex >= size)
                    break;
                sem_wait(sizeOfKitchen);
                sem_wait(mutexKitchen); // mutfakta kimse var mi ...

                sprintf(print, "The supplier supplier is going to the kitchen to deliver main course:  kitchen items P: %d, C: %d, D: %d  = %d\n",getValueFromSem(sem_P), getValueFromSem(sem_C), getValueFromSem(sem_D),totalItems(sem_P,sem_C,sem_D));
                write(STDOUT_FILENO, print, strlen(print));

                sem_post(sem_C);
                mainCourseIndex++;
                sprintf(print, "The supplier delivered main course - after delivery:  kitchen items P: %d, C: %d, D: %d  = %d\n",getValueFromSem(sem_P), getValueFromSem(sem_C), getValueFromSem(sem_D),totalItems(sem_P,sem_C,sem_D));
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(mutexKitchen); // Mutfaga baska biri girebilir ...
                break;
            }
            case DESERT:
            {
                if(desertIndex >= size)
                    break;
                sem_wait(sizeOfKitchen);
                sem_wait(mutexKitchen); // mutfakta kimse var mi ...

                sprintf(print, "The supplier supplier is going to the kitchen to deliver main desert:  kitchen items P: %d, C: %d, D: %d  = %d\n",getValueFromSem(sem_P), getValueFromSem(sem_C), getValueFromSem(sem_D),totalItems(sem_P,sem_C,sem_D));
                write(STDOUT_FILENO,print,strlen(print));
                
                sem_post(sem_D);
                desertIndex++;
                sprintf(print, "The supplier delivered desert - after delivery:  kitchen items P: %d, C: %d, D: %d  = %d\n",getValueFromSem(sem_P), getValueFromSem(sem_C), getValueFromSem(sem_D),totalItems(sem_P,sem_C,sem_D));
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(mutexKitchen); // Mutfaga baska biri girebilir ...
                break;
            }

            default:
                break;
        }    
    }  
    sprintf(print, "Supplier finished supplying - GOODBYE! - # of total items at kitchen : %d\n",totalItems(sem_P,sem_C,sem_D));
    write(STDOUT_FILENO,print,strlen(print));

    if(close(fd) == -1) {
        errorExit("ERROR : filepath closed ");
    }
}

int theCook(sem_t* sem_P_fromSupplier, sem_t* sem_C_fromSupplier, sem_t* sem_D_fromSupplier, sem_t* sem_P_toCounter, sem_t* sem_C_toCounter, sem_t* sem_D_toCounter, 
            sem_t* sizeofKitchen, sem_t* mutexKitchen, sem_t* mutexCook, sem_t* mutexCounter, sem_t* counterSize, sem_t* counterSoupSize, sem_t* counterMainCourseSize, sem_t* counterDesertSize, int size, int N) {
    
    
    if(getValueFromSem(mutexKitchen) == 0) {
        sprintf(print, "Cook %d is waiting at the kitchen - items at kitchen: %d\n",getpid() % N, totalItems(sem_P_fromSupplier,sem_C_fromSupplier,sem_D_fromSupplier));
        write(STDOUT_FILENO,print,strlen(print));
    }
    
    int control = 0;
    int control2 = 0;
    
    
    while(TRUE) {
        
        sem_wait(mutexCook);            
        int totalItem = totalItems(sem_P_toCounter, sem_C_toCounter, sem_D_toCounter);
        if(totalItem == 3 * size) { // while loop exit condition ...
            sem_post(mutexCook);            
            break;
        }

        if(control2 == 0) {
            sprintf(print, "Cook %d is going to the kitchen to wait for/get a plate - kitchen items P: %d, C: %d, D: %d = %d\n",getpid() % N, 
                getValueFromSem(sem_P_fromSupplier), getValueFromSem(sem_C_fromSupplier), getValueFromSem(sem_D_fromSupplier), totalItems(sem_P_fromSupplier,sem_C_fromSupplier,sem_D_fromSupplier));
            write(STDOUT_FILENO, print, strlen(print));
            control2 = 1;
        }
        
        sem_wait(mutexCounter); 
        switch (returnLeastObject(counterSoupSize, counterMainCourseSize, counterDesertSize))
        {
            case SOUP:
            {   
                if(getValueFromSem(counterSize) == 0) {
                    sem_post(mutexCounter); 
                    break;
                }
                
                sem_wait(counterSize);  
               
                sem_wait(sem_P_fromSupplier); 
                sem_wait(mutexKitchen);
                sem_post(sizeofKitchen);
                
                sem_post(mutexKitchen);
               
                sprintf(print, "Cook %d is going to the counter to deliver soup - counter items P: %d, C: %d, D: %d = %d\n",getpid() % N,getValueFromSem(counterSoupSize), 
                        getValueFromSem(counterMainCourseSize), getValueFromSem(counterDesertSize), totalItems(counterSoupSize,counterMainCourseSize,counterDesertSize));
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(counterSoupSize); 
                sem_post(sem_P_toCounter); 
                control = 0;
                control2 = 0;
                sprintf(print, "Cook %d placed soup on the counter        - counter items P: %d, C: %d, D: %d = %d\n",getpid() % N, getValueFromSem(counterSoupSize), 
                        getValueFromSem(counterMainCourseSize), getValueFromSem(counterDesertSize), totalItems(counterSoupSize,counterMainCourseSize,counterDesertSize));                
                write(STDOUT_FILENO,print,strlen(print)); 
                sem_post(mutexCounter); 
                break;
            }
            case MAIN_COURSE:
            {   
                if(getValueFromSem(counterSize) == 0) {
                    sem_post(mutexCounter); 
                    break;
                } 
                sem_wait(counterSize);  
                sem_wait(sem_C_fromSupplier); 
                sem_wait(mutexKitchen);
                sem_post(sizeofKitchen);
                sem_post(mutexKitchen);

                sprintf(print,"Cook %d is going to the counter to deliver main course - counter items P: %d, C: %d, D: %d = %d\n",getpid() % N,getValueFromSem(counterSoupSize), 
                        getValueFromSem(counterMainCourseSize), getValueFromSem(counterDesertSize), totalItems(counterSoupSize,counterMainCourseSize,counterDesertSize));
                write(STDOUT_FILENO,print,strlen(print)); 
                sem_post(counterMainCourseSize); 
                sem_post(sem_C_toCounter); 
                control = 0;
                control2 = 0;
                sprintf(print,"Cook %d placed main course on the counter - counter items P: %d, C: %d, D: %d = %d\n",getpid() % N, getValueFromSem(counterSoupSize), 
                        getValueFromSem(counterMainCourseSize), getValueFromSem(counterDesertSize), totalItems(counterSoupSize,counterMainCourseSize,counterDesertSize));                
                write(STDOUT_FILENO,print,strlen(print)); 
                sem_post(mutexCounter); 
                break;
            }
            case DESERT:
            {   
                if(getValueFromSem(counterSize) == 0) {
                    sem_post(mutexCounter); 
                    break;
                } 
                sem_wait(counterSize);  
                sem_wait(sem_D_fromSupplier); 
                sem_wait(mutexKitchen);
                sem_post(sizeofKitchen);
                sem_post(mutexKitchen);

                sprintf(print,"Cook %d is going to the counter to deliver desert - counter items P: %d, C: %d, D: %d = %d\n",getpid() % N,getValueFromSem(counterSoupSize), 
                        getValueFromSem(counterMainCourseSize), getValueFromSem(counterDesertSize), totalItems(counterSoupSize,counterMainCourseSize,counterDesertSize));
                write(STDOUT_FILENO,print,strlen(print)); 
                sem_post(counterDesertSize); 
                sem_post(sem_D_toCounter); 
                control = 0;
                control2 = 0;
                sprintf(print,"Cook %d placed desert on the counter      - counter items P: %d, C: %d, D: %d, at kitchen: %d\n",getpid() % N,getValueFromSem(counterSoupSize), 
                        getValueFromSem(counterMainCourseSize), getValueFromSem(counterDesertSize), totalItems(counterSoupSize,counterMainCourseSize,counterDesertSize));
                write(STDOUT_FILENO,print,strlen(print)); 
                sem_post(mutexCounter); 
                break;
            }
            default:
                break;
        }
        sem_post(mutexCook);            
    }
    sprintf(print,"Cook %d finished serving - items at kitchen: %d - going home - GOODBYE!!!\n",getpid() % N, totalItems(sem_P_fromSupplier,sem_C_fromSupplier,sem_D_fromSupplier));
    write(STDOUT_FILENO,print,strlen(print)); 
}


int theGraduated(sem_t* counterSoupSize, sem_t* counterMainCourseSize, sem_t* counterDesertSize, sem_t* counterSize, sem_t* mutexCounter, sem_t* mutexStudentsGraduated, sem_t* emptyTable, sem_t* mutexGraduated, sem_t* totalStudents, int limit, int G) {
    
    int control = 0;
    for(int i = 1; i <= limit; i++) {   
        sem_wait(totalStudents);
        sem_wait(mutexGraduated);
        sem_wait(mutexStudentsGraduated);
        
        int soupSize = getValueFromSem(counterSoupSize);
        int mainCourseSize = getValueFromSem(counterMainCourseSize);
        int desertSize = getValueFromSem(counterDesertSize);
        
        if(soupSize > 0 && mainCourseSize > 0 && desertSize > 0) {
            
            sprintf(print, "Graduated %d is going to the counter (round  %d) - # of students at counter: %d and counter items P:%d, C:%d, D:%d = %d\n",getpid() % G, i, getValueFromSem(totalStudents), 
                soupSize, mainCourseSize, desertSize,soupSize+mainCourseSize+desertSize);
            write(STDOUT_FILENO,print,strlen(print));

            control = 0;
            sem_wait(counterSoupSize);
            sem_wait(counterMainCourseSize);
            sem_wait(counterDesertSize);
            sem_wait(mutexCounter);
            
            sem_post(counterSize);
            sem_post(counterSize);
            sem_post(counterSize);
            sem_post(mutexCounter);
            sem_post(totalStudents);
            
            sem_post(mutexStudentsGraduated);
            sem_post(mutexGraduated);
            
            sprintf(print, "Graduated %d got food and is going to get a table (round %d) - # of empty tables: %d\n",getpid() % G, i, getValueFromSem(emptyTable));
            write(STDOUT_FILENO,print,strlen(print));
            
            if(getValueFromSem(emptyTable) == 0) {
                sprintf(print, "Graduated %d is waiting for a table (round %d) - # of empty tables: %d\n", getpid() % G, i, getValueFromSem(emptyTable));
                write(STDOUT_FILENO,print,strlen(print));
            }
            else {
                sem_wait(emptyTable);
                sprintf(print,"Graduated %d sat at table %d to eat (round %d) - on counter P:%d, C:%d, D:%d, empty tables: %d\n",getpid() % G, getValueFromSem(emptyTable) + 1, i, getValueFromSem(counterSoupSize),getValueFromSem(counterMainCourseSize),getValueFromSem(counterDesertSize),getValueFromSem(emptyTable));
                write(STDOUT_FILENO,print,strlen(print));
                sprintf(print, "Graduated %d left table %d to eat again (round %d) - empty tables: %d\n",getpid() % G, getValueFromSem(emptyTable) + 1, i, getValueFromSem(emptyTable) + 1);
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(emptyTable);
            }
                
        } else {
            
            i--;
            sem_post(totalStudents);
            sem_post(mutexStudentsGraduated);
            sem_post(mutexGraduated);
        }
        
    }
    sem_wait(totalStudents);
    sprintf(print, "Graduated %d is done eating L=%d times - going home - GOODBYE!!!\n",getpid() % G, limit);
    write(STDOUT_FILENO,print,strlen(print));
}

int theStudent(sem_t* counterSoupSize, sem_t* counterMainCourseSize, sem_t* counterDesertSize, sem_t* counterSize, sem_t* mutexCounter, sem_t* mutexStudents, sem_t* emptyTable, sem_t* mutexGraduated, sem_t* totalStudents, int limit, int U) {
    
    int control = 0;
    for(int i = 1; i <= limit; i++) {   
        sem_wait(totalStudents); 
        sem_wait(mutexStudents);
        sem_wait(mutexGraduated);
       
        int soupSize = getValueFromSem(counterSoupSize);
        int mainCourseSize = getValueFromSem(counterMainCourseSize);
        int desertSize = getValueFromSem(counterDesertSize);
        
        if(soupSize > 0 && mainCourseSize > 0 && desertSize > 0) {
            
            control = 0;
            sprintf(print, "Student %d is going to the counter (round  %d) - # of students at counter: %d and counter items P:%d, C:%d, D:%d = %d\n",getpid() % U, i, getValueFromSem(totalStudents), 
                soupSize, mainCourseSize, desertSize,soupSize+mainCourseSize+desertSize);
            write(STDOUT_FILENO,print,strlen(print));

            sem_wait(counterSoupSize);
            sem_wait(counterMainCourseSize);
            sem_wait(counterDesertSize);
            sem_wait(mutexCounter);
            
            sem_post(counterSize);
            sem_post(counterSize);
            sem_post(counterSize);
            sem_post(mutexCounter);
            sem_post(totalStudents);
            
            sem_post(mutexGraduated);
            sem_post(mutexStudents);
            
            
            sprintf(print, "Student %d got food and is going to get a table (round %d) - # of empty tables: %d\n",getpid() % U, i, getValueFromSem(emptyTable));
            write(STDOUT_FILENO,print,strlen(print));

            if(getValueFromSem(emptyTable) == 0) {
                sprintf(print, "Student %d is waiting for a table (round %d) - # of empty tables: 0\n",getpid() % U, i, getValueFromSem(emptyTable));
                write(STDOUT_FILENO,print,strlen(print));
            }
            else {
                sem_wait(emptyTable);
                sprintf(print, "Student %d sat at table %d to eat (round %d) - empty tables: %d\n",getpid() % U, getValueFromSem(emptyTable) + 1, i,getValueFromSem(emptyTable));
                write(STDOUT_FILENO,print,strlen(print));
                sprintf(print, "Student %d left table %d to eat again (round %d) - empty tables: %d\n",getpid() % U, getValueFromSem(emptyTable) + 1, i, getValueFromSem(emptyTable) + 1);
                write(STDOUT_FILENO,print,strlen(print));
                sem_post(emptyTable);
            }
                
        } else {
            i--;
            sem_post(totalStudents);
            sem_post(mutexGraduated);
            sem_post(mutexStudents);
            
        }
        
    }
    sem_wait(totalStudents);
    sprintf(print, "Student %d is done eating L=%d times - going home - GOODBYE!!!\n",getpid() % U, limit);
    write(STDOUT_FILENO,print,strlen(print));
}


int ifKitchenHasNoSize(sem_t* semp) {
    int result;
    sem_getvalue(semp, &result);
    if(result == 0)
        return TRUE;
    return FALSE;
}

int totalItems(sem_t* sem_P, sem_t* sem_C, sem_t* sem_D) {
    int p_size,c_size,d_size;
    sem_getvalue(sem_P,&p_size);
    sem_getvalue(sem_C,&c_size);
    sem_getvalue(sem_D,&d_size);
    return (p_size + c_size + d_size);
}

int returnLeastObject(sem_t* sem_P, sem_t* sem_C, sem_t* sem_D) {
    int p = 0, c = 0, d = 0;
    sem_getvalue(sem_P,&p);
    sem_getvalue(sem_C,&c);
    sem_getvalue(sem_D,&d);
    
    if (p < c) {
        if (p < d)
            return SOUP;
        else if (p == d)
            return SOUP;
        else 
            return DESERT;
    }
    else if (p == c) {
        if (p < d)
            return SOUP;
        else if (p == d)
            return SOUP;
        else
            return DESERT;
    }
    else {
        if (c < d)
            return MAIN_COURSE;
        
        else if (c == d)
            return DESERT;
        
        else
            return DESERT;
    }
}

int getValueFromSem(sem_t* sem) {
    int a;
    sem_getvalue(sem,&a);
    return a;
}

int getRandomPlate(int fd) {
    char readedByte[1];

    if(read(fd,readedByte,1) == -1) {
        errorExit("ERROR read file");
    }

    if(readedByte[0] == 'p' || readedByte[0] == 'P' )
        return SOUP;
    if(readedByte[0] == 'c' || readedByte[0] == 'C' )
        return MAIN_COURSE;
    if(readedByte[0] == 'd' || readedByte[0] == 'D' )
        return DESERT;
}

size_t getFileSize(const char* fileName) {
    struct stat statX;
    stat(fileName, &statX);
    size_t size = statX.st_size;
    return size;
}