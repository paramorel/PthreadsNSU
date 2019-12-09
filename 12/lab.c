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
    pthread_mutex_t mutex;
    pthread_cond_t conditional;
    int changeThread;
} SharedData;

void cleanResources(SharedData*);
void* printMessage(void*);
void exitBecauseError(int, char*);
void initSharedData(SharedData*);
void lockMutex(SharedData*);
void unlockMutex(SharedData*);
void unlockConditional(SharedData*);

void exitBecauseError(int errorCode, char* message){
    if (0 != errorCode){
        if (NULL == message){
            message = "error message ";
        }

        fprintf(stderr, message, strerror(errorCode));
        exit(EXIT_FAILURE);
    }
}

void unlockConditional(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    errorCode = pthread_cond_broadcast(&(sharedData->conditional));
    exitBecauseError(errorCode, "pthread_cond_signal error");
}

void lockMutex(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    errorCode = pthread_mutex_lock(&(sharedData->mutex));
    exitBecauseError(errorCode, "pthread_mutex_lock error ");
}

void unlockMutex(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    errorCode = pthread_mutex_unlock(&(sharedData->mutex));
    exitBecauseError(errorCode, "pthread_mutex_unlock error ");
}

void cleanResources(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if (0 != (errorCode = pthread_mutex_destroy(&(sharedData->mutex)))){
        errno = errorCode;
        perror("pthread_mutex_destroy error");
    }

    if (0 != (errorCode = pthread_cond_destroy(&(sharedData->conditional)))){
        errno = errorCode;
        perror("pthread_cond_destroy error");
        exit(EXIT_FAILURE);
    }
    free(sharedData);
}

void* printMessage(void* threadData){
    int errorCode = 0;
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;

    lockMutex(sharedData);

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        sharedData->changeThread = 0;
        unlockConditional(sharedData);

	    fprintf(stdout, "Child thread\n");

        while(!sharedData->changeThread){
            errorCode = pthread_cond_wait(&(sharedData->conditional), &(sharedData->mutex));
        }
        exitBecauseError(errorCode, "pthread_cond_wait error");
    }

    sharedData->changeThread = 0;
    unlockMutex(sharedData);
    unlockConditional(sharedData);

    return NULL;
}

void initSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;

    errorCode = pthread_mutex_init(&(sharedData->mutex), NULL);
    exitBecauseError(errorCode, "pthread_mutex_init error ");

    errorCode = pthread_cond_init(&(sharedData->conditional), NULL);
    exitBecauseError(errorCode, "pthread_cond_init error ");
    sharedData->changeThread = 0;
}


int main(int argc, char *argv[]){
    int errorCode = 0;
    pthread_t thread;

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    initSharedData(sharedData);

    lockMutex(sharedData);

    if (0 != (errorCode = pthread_create(&thread, NULL, printMessage, (void*)sharedData))) {
        errno = errorCode;
        
        perror("pthread_create error");
        unlockMutex(sharedData);
        cleanResources(sharedData);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        sharedData->changeThread = 1;
        unlockConditional(sharedData);

	    fprintf(stdout, "Main thread\n");

        while(sharedData->changeThread){
            errorCode = pthread_cond_wait(&(sharedData->conditional), &(sharedData->mutex));
        }   
        exitBecauseError(errorCode, "pthread_cond_wait error");
    }
    sharedData->changeThread = 1;

    unlockMutex(sharedData);
    unlockConditional(sharedData);

    if (0 != (errorCode = pthread_join(thread, NULL))){
        errno = errorCode;
        perror("pthread_join error");
        return EXIT_FAILURE;
    }

    cleanResources(sharedData);

    return EXIT_SUCCESS;
}