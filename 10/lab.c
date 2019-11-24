#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT_OF_LINES_TO_PRINT 10
#define COUNT_OF_MUTEXES 3 
#define COUNT_OF_THREADS 2 

typedef struct SharedData{
    int childStarted;
    pthread_mutex_t mutex[COUNT_OF_MUTEXES];
    pthread_mutexattr_t mutexAttr;
} SharedData;

void initMutexes(SharedData*);
void cleanResources(SharedData*);
void* printMessage(void*);
void lockMutex(pthread_mutex_t*, SharedData*);
void unlockMutex(pthread_mutex_t*, SharedData*);
void exitBecauseError(int, char*, SharedData*);


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


void *printMessage(void *threadData){
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;
    
    lockMutex(&(sharedData->mutex[2]), sharedData);
    int k = 2;
    sharedData->childStarted = 1;
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&(sharedData->mutex[(k + 1) % COUNT_OF_MUTEXES]), sharedData);

        fprintf(stdout, "Child thread\n");

        unlockMutex(&(sharedData->mutex[k]), sharedData);
        k = (k + 1) % COUNT_OF_MUTEXES;
    }

    unlockMutex(&(sharedData->mutex[k]), sharedData);
    return NULL;
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
    for (int i = 0; i < COUNT_OF_MUTEXES; i++){
        if (0 != (errorCode = pthread_mutex_destroy(&(sharedData->mutex[i])))){
            errno = errorCode;
            perror("pthread_mutex_destroy error");
        }
    }
}

void initMutexes(SharedData* sharedData){
    assert(NULL != sharedData);

    int errorCode = 0;
    if(0 != (errorCode = pthread_mutexattr_init(&(sharedData->mutexAttr)))){
        errno = errorCode;
        perror("pthread_mutexattr_init error");
        free(sharedData);
        exit(EXIT_FAILURE);
    }

    if (0 != (errorCode = pthread_mutexattr_settype(&(sharedData->mutexAttr), PTHREAD_MUTEX_ERRORCHECK))){
        errno = errorCode;
        perror("pthread_mutexattr_settype");
        
        if (0 != (errorCode = pthread_mutexattr_destroy(&(sharedData->mutexAttr)))){
            errno = errorCode;
            perror("pthread_mutexattr_destroy after settype error");
        }

        free(sharedData);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < COUNT_OF_MUTEXES; i++){
        if (0 != (errorCode = pthread_mutex_init(&(sharedData->mutex[i]), &(sharedData->mutexAttr)))){
            errno = errorCode;
            perror("pthread_mutex_init error");
            for (int j = 0; j < i; j++){
                if (0 != (errorCode = pthread_mutex_destroy(&(sharedData->mutex[i])))){
                    errno = errorCode;
                    perror("pthread_mutex_destroy after init error");
                }
            }
            if (0 != (errorCode = pthread_mutexattr_destroy(&(sharedData->mutexAttr)))){
                errno = errorCode;
                perror("pthread_mutexattr_destroy after init error");
            }
            free(sharedData);
            exit(EXIT_FAILURE);
        }
    }
}


int main(int argc, char *argv[]) {
    int errorCode = 0;
    pthread_t thread;

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    sharedData->childStarted = 0;

    initMutexes(sharedData);

    lockMutex(&(sharedData->mutex[0]), sharedData);

    if (0 != (errorCode = pthread_create(&thread, NULL, printMessage, (void*)sharedData))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanResources(sharedData);
        free(sharedData);
        return EXIT_FAILURE;
    }


    while (!sharedData->childStarted){
        if(0 !=  sleep(0)){
            fprintf(stderr, "defective sleep");
            cleanResources(sharedData);
            free(sharedData);
            return EXIT_FAILURE;
        }
    }

    int k = 0;
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&(sharedData->mutex[(k + 1) % COUNT_OF_MUTEXES]), sharedData);

        fprintf(stdout, "Main thread\n");

        unlockMutex(&(sharedData->mutex[k]), sharedData);
        k = (k + 1) % COUNT_OF_MUTEXES;
    }

    unlockMutex(&(sharedData->mutex[k]), sharedData);

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