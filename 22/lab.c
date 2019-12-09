#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define TIME_FOR_A 1
#define TIME_FOR_B 2
#define TIME_FOR_C 3
#define COUNT_OF_THREADS 5
#define COUNT_OF_WIDGETS 5
#define MAX_LENGTH_OF_INPUT_STRING 10
#define CLEANUP_POP_ARGUMENT 1

typedef struct SharedData{
    sem_t semaphoreA;
    sem_t semaphoreB;
    sem_t semaphoreC;
    sem_t semaphoreAB;
}SharedData;

typedef struct Threads{
    pthread_t threadA;
    pthread_t threadB;
    pthread_t threadC;
    pthread_t threadAB;
}Threads;

void exitBecauseError(int, char*);
void initSharedData(SharedData*);
void cleanResources(SharedData*);
void wait(sem_t*);
void post(sem_t*);
void createWidget(SharedData*, Threads*);
void* createA(void*);
void* createB(void*);
void* createC(void*);
void* createAB(void*);
void cancelThread(pthread_t*);
void cleanup(void*);

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
    if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreA))){
        errno = errorCode;
        perror("sem_destroy A error");
        exit(EXIT_FAILURE);
    }
    
    if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreB))){
        errno = errorCode;
        perror("sem_destroy B error");
        exit(EXIT_FAILURE);
    }
  
    if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreC))){
        errno = errorCode;
        perror("sem_destroy C error");
        exit(EXIT_FAILURE);
    }
   
    if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreAB))){
        errno = errorCode;
        perror("sem_destroy AB error");
        exit(EXIT_FAILURE);
    }
    free(sharedData);
}

void initSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0!= (errorCode = sem_init(&(sharedData->semaphoreA), 0, 0))){
        errno = errorCode;
        perror("sem_init A error");
        free(sharedData);
        exit(EXIT_FAILURE);
    } 

    if(0 != (errorCode = sem_init(&(sharedData->semaphoreB), 0, 0))){
        errno = errorCode;
        perror("sem_init B error");
        if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreA))){
            errno = errorCode;
            perror("sem_destroy A error");
            exit(EXIT_FAILURE);
        }    
        free(sharedData);  
        exit(EXIT_FAILURE);
    } 
    if(0 != (errorCode = sem_init(&(sharedData->semaphoreC), 0, 0))){
        errno = errorCode;
        perror("sem_init C error");
        if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreA))){
            errno = errorCode;
            perror("sem_destroy A error");
        }
        if(0 != (errorCode = sem_destroy(&sharedData->semaphoreB))){
            errno = errorCode;
            perror("sem_destroy B error");
            exit(EXIT_FAILURE);
        }
        free(sharedData);
        exit(EXIT_FAILURE);
    } 
    if(0 != (errorCode = sem_init(&(sharedData->semaphoreAB), 0, 0))){
        errno = errorCode;
        perror("sem_init AB error");
        if(0 !=(errorCode = sem_destroy(&sharedData->semaphoreA))){
            errno = errorCode;
            perror("sem_destroy A error");
        }
        if(0 != (errorCode = sem_destroy(&sharedData->semaphoreB))){
            errno = errorCode;
            perror("sem_destroy B error");
            exit(EXIT_FAILURE);
        }
        if(0 != (errorCode = sem_destroy(&sharedData->semaphoreC))){
            errno = errorCode;
            perror("sem_destroy C error");
            exit(EXIT_FAILURE);
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

void* createA (void* threadData){
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;
    char* cleanupText = "thread A canceled";
    pthread_cleanup_push(cleanup, cleanupText);
    while(1){
	    sleep(TIME_FOR_A);
	    post(&sharedData->semaphoreA);    
	    fprintf(stdout, "New detail A\n");
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
    return NULL;
}

void* createB (void* threadData){
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;
    char* cleanupText = "thread B canceled";
    pthread_cleanup_push(cleanup, cleanupText);
    while(1){
	    sleep(TIME_FOR_B);
	    post(&sharedData->semaphoreB);    
	    fprintf(stdout, "New detail B\n");
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
    return NULL;
}

void* createC (void* threadData){
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;
    char* cleanupText = "thread C canceled";
    pthread_cleanup_push(cleanup, cleanupText);
    while(1){
	    sleep(TIME_FOR_C);
	    post(&sharedData->semaphoreC);    
	    fprintf(stdout, "New detail C\n");
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
    return NULL;
}

void* createAB (void* threadData){
    assert(NULL != threadData);
    SharedData* sharedData = (SharedData*)threadData;
    char* cleanupText = "thread AB canceled";
    pthread_cleanup_push(cleanup, cleanupText);
    while(1){
        pthread_testcancel();
	    wait(&sharedData->semaphoreA);
	    wait(&sharedData->semaphoreB);
	    post(&sharedData->semaphoreAB);
	    fprintf(stdout, "New module AB\n");
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
    return NULL;
}

void createWidget(SharedData* sharedData, Threads* threads){
    assert(NULL != sharedData);
    assert(NULL !=threads);
    int countOfWidgets = 0;
    while(countOfWidgets < COUNT_OF_WIDGETS){
        countOfWidgets++;
	    wait(&sharedData->semaphoreAB);
	    wait(&sharedData->semaphoreC);
	    fprintf(stdout, "New widget\n");
    }
    cancelThread(&threads->threadA);
    cancelThread(&threads->threadB);
    cancelThread(&threads->threadC);
    cancelThread(&threads->threadAB);

    cleanResources(sharedData);
}

void cleanup(void* text){
    assert(NULL != text);
    char* cleanupText = (char*)text;
    fprintf(stdout, "%s\n", cleanupText);
}

void cancelThread(pthread_t* thread){
    int errorCode = 0;
    assert(NULL != thread);
    errorCode = pthread_cancel(*thread);
    exitBecauseError(errorCode, "pthread_cancel error");
    errorCode = pthread_join(*thread, NULL);
    exitBecauseError(errorCode, "pthread_join error");
}

int main(int argc, char*argv[]){
    int errorCode = 0;

    Threads* threads = (Threads*)malloc(sizeof(Threads));

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);
    char* inputString = malloc(MAX_LENGTH_OF_INPUT_STRING * sizeof(char));

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    initSharedData(sharedData);


    if (0 != (errorCode = pthread_create(&threads->threadA, NULL, createA, (void*)sharedData))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanResources(sharedData);
        return EXIT_FAILURE;
    }
    errorCode = pthread_create(&threads->threadB, NULL, createB, (void*)sharedData);
    exitBecauseError(errorCode, "pthread_create B error");

    errorCode = pthread_create(&threads->threadC, NULL, createC, (void*)sharedData);
    exitBecauseError(errorCode, "pthread_create C error");

    errorCode = pthread_create(&threads->threadAB, NULL, createAB, (void*)sharedData);
    exitBecauseError(errorCode, "pthread_create AB error");

    createWidget(sharedData, threads);

    free(inputString);
    free(threads);

    fprintf(stdout,"All structures deleted successfully\n");
    return EXIT_SUCCESS;
}