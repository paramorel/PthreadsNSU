#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <errno.h>
#include <assert.h>

#define COUNT_OF_LINES_TO_PRINT 10
#define NUMBER_OF_PROCESSES 2 

typedef struct SharedData{
    sem_t* semaphore1;
    sem_t* semaphore2;
    char* nameForSemaphore1;
    char* nameForSemaphore2;
} SharedData;

void exitBecauseError(int, char*);
void initSharedData(SharedData*);
void waitSemaphore(sem_t*);
void post(sem_t*);
void closeSemaphores(SharedData*);
void unlinkSemaphores(SharedData*);

void exitBecauseError(int errorCode, char* message){
    if (0 != errorCode){
        if (NULL == message){
            message = "error message ";
        }
        fprintf(stderr, message, strerror(errorCode));
        exit(EXIT_FAILURE);
    }
}

void initSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    sharedData->nameForSemaphore1 = "/semaphore1";
    sharedData->nameForSemaphore2 = "/semaphore2";
   
    sharedData->semaphore1 = sem_open(sharedData->nameForSemaphore1, O_CREAT, S_IRWXU, 1);
    if(SEM_FAILED == sharedData->semaphore1){
        perror("sem_open 1 error");
        free(sharedData);
        exit(EXIT_FAILURE);
    }

    sharedData->semaphore2 = sem_open(sharedData->nameForSemaphore2, O_CREAT, S_IRWXU, 0);
    if(SEM_FAILED == sharedData->semaphore2){
        perror("sem_open 2 error");
        free(sharedData);
        exit(EXIT_FAILURE);
    }
}

void waitSemaphore(sem_t* semaphore){
    assert(NULL != semaphore);
    int errorCode = 0;
    errorCode = sem_wait(semaphore);
    exitBecauseError(errorCode, "sem_wait error");
}

void post(sem_t* semaphore){
    assert(NULL != semaphore);
    int errorCode = 0;
    errorCode = sem_post(semaphore);
    exitBecauseError(errorCode, "sem_post error");
}

void closeSemaphores(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0 != (errorCode = sem_close(sharedData->semaphore1))){
        errno = errorCode;
        perror("sem_close 1 error");
    }
    if(0 != (errorCode = sem_close(sharedData->semaphore2))){
        errno = errorCode;
        perror("sem_close 2 error");
    }
}

void unlinkSemaphores(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0 != (errorCode = sem_unlink(sharedData->nameForSemaphore1))){
        errno = errorCode;
        perror("sem_unlink 1 error");
    }
    if(0 != (errorCode = sem_unlink(sharedData->nameForSemaphore2))){
        errno = errorCode;
        perror("sem_unlink 2 error");
    }
}

void parentPrintsMessage(SharedData* sharedData){
    assert(NULL != sharedData);
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        waitSemaphore(sharedData->semaphore1);
        fprintf(stdout, "Parent process\n");
        post(sharedData->semaphore2);
    }  
}

void childPrintsMessage(SharedData* sharedData){
    assert(NULL != sharedData);
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        waitSemaphore(sharedData->semaphore2);
        fprintf(stdout, "Child process\n");
        post(sharedData->semaphore1);
    }
}

int main(int argc, char* argv[]){	
    int returnedValue = 0;
    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData));

    initSharedData(sharedData);
	
    returnedValue = fork();

    if (returnedValue == -1){
        errno = returnedValue;
        perror("fork error");
        free(sharedData);
        return EXIT_FAILURE;

    }else if (returnedValue == 0){
        childPrintsMessage(sharedData);

    }else{
        parentPrintsMessage(sharedData);
    }

    if(0 != returnedValue){
        closeSemaphores(sharedData);
        unlinkSemaphores(sharedData);
        free(sharedData);
    }else{
        closeSemaphores(sharedData);
    }

	return EXIT_SUCCESS;
}