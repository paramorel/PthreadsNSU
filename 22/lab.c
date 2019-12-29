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
#define COUNT_OF_THREADS 4
#define COUNT_OF_WIDGETS 5
#define CLEANUP_POP_ARGUMENT 1
#define SUCCESS 0
#define FAILURE -1
#define A_ID 0
#define B_ID 1
#define C_ID 2
#define MODULE_ID 3
#define WIDGET_ID 4

typedef struct Semaphores{
    sem_t* semaphore[COUNT_OF_THREADS];
}Semaphores;

typedef struct LocalThreadData{
    Semaphores* allSemaphores;
    int threadID;
}LocalThreadData;

int initLocalThreadData(LocalThreadData*);
void* creator(void*);
int createWidget(Semaphores*);
int createDetail(LocalThreadData*);
int createModule(LocalThreadData*);
void cleanup(void*);
void destroySemaphore(sem_t*);

void cleanup(void* text){
    assert(NULL != text);
    char* cleanupText = (char*)text;
    fprintf(stdout, "Thread that makes %s is canceled\n", cleanupText);
}

int post(sem_t* semaphore){
    int errorCode = 0;
    if (0 != (errorCode = sem_post(semaphore))){
        errno = errorCode;
        perror("sem_init error");
        return FAILURE;
    }
    return SUCCESS;
}

int wait(sem_t* semaphore){
    int errorCode = 0;
    if (0 != (errorCode = sem_wait(semaphore))){
        errno = errorCode;
        perror("sem_wait error");
        return FAILURE;
    }
    return SUCCESS;
}

int createDetail(LocalThreadData* localThreadData){//wait(detailSemaphore);
    assert(NULL != localThreadData);
    int timeToSleep = 0;
    int errorCode = 0;
    int countInStock = 0;
    char* nameOfDetail = NULL;
    int threadID = localThreadData->threadID;
    sem_t* mySemaphore = localThreadData->allSemaphores->semaphore[threadID];

    if (A_ID == threadID){
        timeToSleep = TIME_FOR_A;
        nameOfDetail = "A";
    } else if (B_ID == threadID){
        timeToSleep = TIME_FOR_B;
        nameOfDetail = "B";
    } else {
        timeToSleep = TIME_FOR_C;
        nameOfDetail = "C";
    }
    pthread_cleanup_push(cleanup, nameOfDetail);
    while(1){
	    sleep(timeToSleep);

	    if (SUCCESS != post(mySemaphore)){
            return FAILURE;
        } 
        if (0 != (errorCode = sem_getvalue(mySemaphore, &countInStock))){
            errno = errorCode;
            perror("error: sem_getvalue(detail semaphore) in createDetail");
        }
        fprintf(stdout, "%s IN STOCK %d (FROM CREATE DETAILS)\n", nameOfDetail, countInStock);

	    fprintf(stdout, "New %s\n", nameOfDetail);
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
}

int createModule(LocalThreadData* localThreadData){//wait(A), wait(B), post(AB)
    assert(NULL != localThreadData);
    int errorCode = 0;
    int countInStock = 0;
    char* cleanupText = "module from parts A and B";
    sem_t* moduleSemaphore = localThreadData->allSemaphores->semaphore[MODULE_ID];
    sem_t* semaphoreA = localThreadData->allSemaphores->semaphore[A_ID];
    sem_t* semaphoreB = localThreadData->allSemaphores->semaphore[B_ID];
    pthread_cleanup_push(cleanup, cleanupText);
    while(1){
        pthread_testcancel();

	    if (SUCCESS != wait(semaphoreA)){
            return FAILURE;
        }
        if (0 != (errorCode = sem_getvalue(semaphoreA, &countInStock))){
            errno = errorCode;
            perror("error: sem_getvalue(semaphoreA) in createModule");
        }
        fprintf(stdout, "A IN STOCK %d (FROM CREATE MODULE)\n", countInStock);

	    if (SUCCESS != wait(semaphoreB)){
            return FAILURE;
        }
        if (0 != (errorCode = sem_getvalue(semaphoreB, &countInStock))){
            errno = errorCode;
            perror("error: sem_getvalue(semaphoreB) in createModule");
        }
        fprintf(stdout, "B IN STOCK %d (FROM CREATE MODULE)\n", countInStock);

	    if (SUCCESS != post(moduleSemaphore)){
            return FAILURE;
        }
        if (0 != (errorCode = sem_getvalue(moduleSemaphore, &countInStock))){
            errno = errorCode;
            perror("error: sem_getvalue(moduleSemaphore) in createModule");
        }
        fprintf(stdout, "MODULES IN STOCK %d (FROM CREATE MODULE)\n", countInStock);

	    fprintf(stdout, "New module AB\n");
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
}

void* creator(void* threadData){
    assert(NULL != threadData);
    LocalThreadData* localThreadData = (LocalThreadData*)threadData;
    int threadID = localThreadData->threadID;

    if (A_ID == threadID || B_ID == threadID || C_ID == threadID){
        if (SUCCESS != createDetail(localThreadData)){
            exit(EXIT_FAILURE);
        }
    } else {
        if (SUCCESS != createModule(localThreadData)){
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}

int createWidget(Semaphores* semaphores){//wait(AB), wait(C)
    assert(NULL != semaphores);
    int countOfWidgets = 0;
    int countInStock = 0;
    int errorCode = 0;
    sem_t* semaphoreC = semaphores->semaphore[C_ID];
    sem_t* semaphoreAB = semaphores->semaphore[MODULE_ID];

    while(COUNT_OF_WIDGETS != countOfWidgets){
        countOfWidgets++;

        if (SUCCESS != wait(semaphoreAB)){
            return FAILURE;
        }
        if (0 != (errorCode =sem_getvalue(semaphoreAB, &countInStock))){
            errno = errorCode;
            perror("error: set_getvalue(semaphoreAB) in createWidget");
        }
        fprintf(stdout, "MODULES IN STOCK %d (FROM CREATE WIDGET)\n", countInStock);


        if (SUCCESS != wait(semaphoreC)){
            return FAILURE;
        }
        if (0 != (errorCode =sem_getvalue(semaphoreC, &countInStock))){
            errno = errorCode;
            perror("error: set_getvalue(semaphoreC) in createWidget");
        }
        fprintf(stdout, "C IN STOCK %d (FROM CREATE WIDGET)\n", countInStock);

        fprintf(stdout, "New widget\n");
    }
    return SUCCESS;
}


void destroySemaphore(sem_t* semaphore){
    assert(NULL != semaphore);
    int errorCode = 0;
    if (0 != (errorCode = sem_destroy(semaphore))){
        errno = errorCode;
        perror("sem_destroy error");
    }
}


int main(int argc, char** argv){
    int errorCode = 0;
    pthread_t threads[COUNT_OF_THREADS];
    LocalThreadData* localThreadData[COUNT_OF_THREADS];
    Semaphores* semaphores;

    semaphores = malloc(sizeof(Semaphores));
    if (NULL == semaphores){
        fprintf(stdout, "Memory allocation error");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        semaphores->semaphore[i] = malloc(sizeof(sem_t));
        if(NULL == semaphores->semaphore[i]){
            fprintf(stderr, "Memory allocation error");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        if (0 != (errorCode = sem_init(semaphores->semaphore[i], 0, 0))){
            perror("sem_init error");
            for(int j = 0; j < i; j++){
                destroySemaphore(semaphores->semaphore[j]);
            }
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        if (NULL == (localThreadData[i] = malloc(sizeof(LocalThreadData)))){
            fprintf(stderr, "Memory allocation error\n");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        localThreadData[i]->threadID = i;
        localThreadData[i]->allSemaphores = semaphores;
        if (0 != (errorCode = pthread_create(&threads[i], NULL, creator, (void*)localThreadData[i]))){
            perror("pthread_create error");
                for (int j = 0; j < i; j++){
                    if (0 != pthread_join(threads[i], NULL)){
                        perror("pthread_join after pthread_create error");
                    }
                }

            for(int i = 0; i < COUNT_OF_THREADS; i++){
                destroySemaphore(semaphores->semaphore[i]);
            }
            free(semaphores);
            for (int i = 0; i < COUNT_OF_THREADS; i++){
                free(localThreadData[i]);
            }
            return EXIT_FAILURE;
        }
    }

    if (SUCCESS != createWidget(semaphores)){
        return EXIT_FAILURE;
    }
    for (int i = 0; i < COUNT_OF_THREADS; i++){
        if (0 != (errorCode = pthread_cancel(threads[i]))){
            errno = errorCode;
            perror("pthread_cancel error");
            return EXIT_FAILURE;
        }
        if (0 != (errorCode = pthread_join(threads[i], NULL))){
            errno = errorCode;
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        destroySemaphore(semaphores->semaphore[i]);
    }
    free(semaphores);
    for (int i = 0; i < COUNT_OF_THREADS; i++){
        free(localThreadData[i]);
    }

    return EXIT_SUCCESS;
}