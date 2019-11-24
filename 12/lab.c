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
} SharedData;


void cleanResources(SharedData*);
void* printMessage(void*);
void exitBecauseError(int, char*, SharedData*);
void initSharedData(SharedData*);


void exitBecauseError(int errorCode, char* message, SharedData* sharedData){
    assert(NULL != sharedData);

    if (0 != errorCode){
        if (NULL == message){
            message = "error message";
        }

        fprintf(stderr, message, strerror(errorCode));
        cleanResources(sharedData);
        free(sharedData);
        exit(EXIT_FAILURE);
    }
}


void lockMutex(pthread_mutex_t* mutex, SharedData* sharedData){
    assert(NULL != sharedData);

    int errorCode = 0;
    errorCode = pthread_mutex_lock(mutex);
    exitBecauseError(errorCode, "pthread_mutex_lock error", sharedData);
}


void unlockMutex(pthread_mutex_t* mutex, SharedData* sharedData){
    assert(NULL != sharedData);

    int errorCode = 0;
    errorCode = pthread_mutex_unlock(mutex);
    exitBecauseError(errorCode, "pthread_mutex_unlock error", sharedData);
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
    }
}


void* printMessage(void* threadData){
    assert(NULL != threadData);

    SharedData* sharedData = (SharedData*)threadData;
    int errorCode = 0;

    lockMutex(&(sharedData->mutex), sharedData);

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        errorCode = pthread_cond_signal(&(sharedData->conditional));
        exitBecauseError(errorCode, "pthread_cond_signal error", sharedData);

	    fprintf(stdout, "Child thread\n");

	    errorCode = pthread_cond_wait(&(sharedData->conditional), &(sharedData->mutex));
        exitBecauseError(errorCode, "pthread_cond_wait error", sharedData);
    }

    unlockMutex(&(sharedData->mutex), sharedData);

    errorCode = pthread_cond_signal(&(sharedData->conditional));
    exitBecauseError(errorCode, "pthread_cond_signal error", sharedData);

    return NULL;
}

void initSharedData(SharedData* sharedData){
    assert(NULL != sharedData);

    int errorCode = 0;

    errorCode = pthread_mutex_init(&(sharedData->mutex), NULL);
    exitBecauseError(errorCode, "pthread_mutex_init error", sharedData);

    errorCode = pthread_cond_init(&(sharedData->conditional), NULL);
    exitBecauseError(errorCode, "pthread_cond_init error", sharedData);
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

    if (0 != (errorCode = pthread_create(&thread, NULL, printMessage, (void*)sharedData))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanResources(sharedData);
        free(sharedData);
        return EXIT_FAILURE;
    }


    lockMutex(&(sharedData->mutex), sharedData);

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        errorCode = pthread_cond_signal(&(sharedData->conditional));
        exitBecauseError(errorCode, "pthread_cond_signal error", sharedData);

	    fprintf(stdout, "Main thread\n");

	    errorCode = pthread_cond_wait(&(sharedData->conditional), &(sharedData->mutex));
        exitBecauseError(errorCode, "pthread_cond_wait error", sharedData);
    }
    unlockMutex(&(sharedData->mutex), sharedData);

    errorCode = pthread_cond_signal(&(sharedData->conditional));
    exitBecauseError(errorCode, "pthread_cond_signal error", sharedData);


    if (0 != (errorCode = pthread_join(thread, NULL))){
        errno = errorCode;
        perror("pthread_join error");
        cleanResources(sharedData);
        free(sharedData);
        return EXIT_FAILURE;
    }

    cleanResources(sharedData);
    free(sharedData);

    return EXIT_SUCCESS;
}