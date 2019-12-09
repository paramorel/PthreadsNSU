#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT_OF_LINES_TO_PRINT 10
#define COUNT_OF_THREADS 2 

typedef struct SharedData{
    sem_t semaphore1;
    sem_t semaphore2;
} SharedData;

void exitBecauseError(int, char*);
void* printMessage(void*);
void initSharedData(SharedData*);
void cleanResources(SharedData*);
void wait(sem_t*);
void post(sem_t*);

void exitBecauseError(int errorCode, char* message){
    if (0 != errorCode){
        if (NULL == message){
            message = "error message ";
        }
        fprintf(stderr, message, strerror(errorCode));
        exit(EXIT_FAILURE);
    }
}

void cleanResources(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0 !=(errorCode = sem_destroy(&sharedData->semaphore1))){
        errno = errorCode;
        perror("sem_destroy 1 error");
    }
    
    if(0 !=(errorCode = sem_destroy(&sharedData->semaphore2))){
        errno = errorCode;
        perror("sem_destroy 2 error");
        exit(EXIT_FAILURE);
    }

    free(sharedData);
}

void initSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0!= (errorCode = sem_init(&(sharedData->semaphore1), 0, 1))){
        errno = errorCode;
        perror("sem_init 1 error");
        free(sharedData);
        exit(EXIT_FAILURE);
    } 

    if(0 != (errorCode = sem_init(&(sharedData->semaphore2), 0, 0))){
        errno = errorCode;
        perror("sem_init 2 error");
        if(0 !=(errorCode = sem_destroy(&sharedData->semaphore1))){
            errno = errorCode;
            perror("sem_destroy 1 error");
        }
        free(sharedData);
        exit(EXIT_FAILURE);
    } 
}

void wait(sem_t* semaphore){
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

void* printMessage(void* threadData){
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        wait(&sharedData->semaphore2);
        fprintf(stdout, "Child thread\n");
        post(&sharedData->semaphore1);
    }
    return NULL;
}

int main(){
    int errorCode = 0;
    pthread_t childThread;

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    initSharedData(sharedData);

    if (0 != (errorCode = pthread_create(&childThread, NULL, printMessage, (void*)sharedData))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanResources(sharedData);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        wait(&(sharedData->semaphore1));
        fprintf(stdout, "Main thread\n");
        post(&(sharedData->semaphore2));
    }

    if (0 != (errorCode = pthread_join(childThread, NULL))){
        errno = errorCode;
        perror("pthread_join error");
        return EXIT_FAILURE;
    }

    cleanResources(sharedData);
    return EXIT_SUCCESS;
}