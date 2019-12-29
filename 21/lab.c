#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT_OF_PHILOSOPHERS 5
#define DELAY 30000
#define FOOD 10
#define SUCCESS 0
#define FAILURE -1
#define FOR_EBUSY -2

typedef struct Synchronization{
    pthread_mutex_t forks[COUNT_OF_PHILOSOPHERS];
    pthread_t philosophers[COUNT_OF_PHILOSOPHERS];
    pthread_mutex_t foodlock;
    pthread_mutex_t startTaking;
    pthread_cond_t conditional;
}Synchronization;

typedef struct LocalThreadData{
    Synchronization* synchronization;
    int philosopherID;
    int spiriousWakeup;
}LocalThreadData;

void* philosopher(void*);
int initLocalData(LocalThreadData*, Synchronization*);
int initSynchroData(Synchronization*);
int destroySynchronization(Synchronization*);
int lockMutex(pthread_mutex_t*);
int unlockMutex(pthread_mutex_t*);
int foodOnTable(Synchronization*);
int getForks(int, int, int, Synchronization*);
int downForks(int, int, int, LocalThreadData*);


int lockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_lock(mutex);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_mutex_lock error");
        return FAILURE;
    }
    return SUCCESS;
}

int unlockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_unlock(mutex);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_mutex_unlock error");
        return FAILURE;
    }
    return SUCCESS;
}

int unlockConditional(pthread_cond_t* conditional){
    int errorCode = 0;
    errorCode = pthread_cond_broadcast(conditional);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_cond_broadcast error");
        return FAILURE;
    }
    return SUCCESS;
}

int getForks(int id, int fork1, int fork2, Synchronization* synchro){
    assert(NULL != synchro);
    int returned;

    if (SUCCESS != lockMutex(&synchro->startTaking)){
        return FAILURE;
    }
    while(returned){
        if (returned = pthread_mutex_trylock(&synchro->forks[fork1])){
           
            returned = pthread_mutex_trylock(&synchro->forks[fork2]);
            if (0 != returned){
                if (SUCCESS != unlockMutex(&synchro->forks[fork1])){
                    return FAILURE;
                }
            }
        }
        if (returned){
            pthread_cond_wait(&synchro->conditional, &synchro->startTaking);  
        }      
    }

    if (SUCCESS != unlockMutex(&synchro->startTaking)){
        return FAILURE;
    }
    fprintf(stdout, "FORK NUMBER %d BUSY BY THREAD %d\n", fork1, id);
    fprintf(stdout, "FORK NUMBER %d BUSY BY THREAD %d\n", fork2, id);
    return SUCCESS;
}

int downForks(int id, int fork1, int fork2, LocalThreadData* localThreadData){
    assert(NULL != localThreadData);
    if (SUCCESS != lockMutex(&localThreadData->synchronization->startTaking)){
        return FAILURE;
    }
    if (SUCCESS != unlockMutex(&localThreadData->synchronization->forks[fork1])){
        return FAILURE;
    }
    fprintf(stdout, "FORK NUMBER %d FREE FROM THREAD %d\n", fork1, id);
    if (SUCCESS != unlockMutex(&localThreadData->synchronization->forks[fork2])){
        return FAILURE;
    }
    fprintf(stdout, "FORK NUMBER %d FREE FROM THREAD %d\n", fork2, id);
    if (SUCCESS != unlockConditional(&localThreadData->synchronization->conditional)){
        return FAILURE;
    }

    localThreadData->spiriousWakeup = 1;

    if (SUCCESS != unlockMutex(&localThreadData->synchronization->startTaking)){
        return FAILURE;
    }
  return SUCCESS;
}

int destroySynchronization(Synchronization* synchro){
    assert(NULL != synchro);
    int errorCode = 0;

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        errorCode = pthread_mutex_destroy(&synchro->forks[i]);
        if (0 != errorCode){
            errno = errorCode;
            perror("pthread_mutex_destroy forks error");
        }
    }

    if (0 != (errorCode = pthread_mutex_destroy(&synchro->foodlock))){
        errno = errorCode;
        perror("pthread_mutex_destroy foodlock error");
    }

    if (0 != (errorCode = pthread_mutex_destroy(&synchro->startTaking))){
        errno = errorCode;
        perror("pthread_mutex_destroy starttaking error");
    }

    if (0 != (errorCode = pthread_cond_destroy(&synchro->conditional))){
        errno = errorCode;
        perror("pthread_cond_destroy error");
    }
}

int initSynchroData(Synchronization* synchronization){
    assert(NULL != synchronization);
    int errorCode = 0;
    
    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        if (0 != (errorCode = pthread_mutex_init(&synchronization->forks[i], NULL))){
            errno = errorCode;
            perror("pthread_mutex_init error");
            return FAILURE;
        }
    }
    
    if (0 != (errorCode = pthread_mutex_init(&synchronization->foodlock, NULL))){
        errno = errorCode;
        perror("pthread_mutex_init foodlock error");
        return FAILURE;
    }

    if (0 != (errorCode = pthread_mutex_init(&synchronization->startTaking, NULL))){
        errno = errorCode;
        perror("pthread_mutex_init startTaking error");
        return FAILURE;
    }

    if (0 != (errorCode = pthread_cond_init(&synchronization->conditional, NULL))){
        errno = errorCode;
        perror("pthread_cond_init error");
        return FAILURE;
    }

    return SUCCESS;
}

void* philosopher(void* threadData){
    assert(NULL != threadData);
    LocalThreadData* localThreadData = (LocalThreadData*)threadData;
    int id = localThreadData->philosopherID;
    Synchronization* synchro = localThreadData->synchronization;
    int errorCode, leftFork, rightFork, food;

    fprintf(stdout, "Philosopher %d sitting down to dinner.\n", id);

    rightFork = id;
    leftFork = id + 1;
 
    if (leftFork == COUNT_OF_PHILOSOPHERS){
        leftFork = 0;
    }

    while(food = foodOnTable(synchro)){
        if (food < 0){
            return localThreadData;
        }

        fprintf (stdout, "Philosopher %d: get dish %d.\n", id, food);
        if (SUCCESS != getForks(id, leftFork, rightFork, synchro)){
            return localThreadData;
        }
        fprintf (stdout, "Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - food + 1));
        fprintf(stdout, "Philosopher %d done now\n", id);

        if (SUCCESS != downForks(id, leftFork, rightFork, localThreadData)){
            return localThreadData;
        }
    }

    fprintf(stdout, "Philosopher %d is done eating.\n", id);

    return NULL;
}

int foodOnTable(Synchronization* synchronization) {
    static int food = FOOD;
    int myfood;

    if (SUCCESS != lockMutex(&synchronization->foodlock)){
        return FAILURE;
    }

    if (food > 0) {
        food--;
    }

    myfood = food;
    if (SUCCESS != unlockMutex(&synchronization->foodlock)){
        return FAILURE;
    }
    return myfood;
}

int main(int argc, char** argv){
    LocalThreadData* localThreadData[COUNT_OF_PHILOSOPHERS];
    static Synchronization* synchro;
    int errorCode = 0;
    int spiriousWakeup = 1;

    synchro = malloc(sizeof(Synchronization));
    if (NULL == synchro){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    if (SUCCESS != initSynchroData(synchro)){
        return EXIT_FAILURE;
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        if (NULL == (localThreadData[i] = malloc(sizeof(LocalThreadData)))){
            fprintf(stderr, "Memory allocation error\n");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        localThreadData[i]->philosopherID = i;
        localThreadData[i]->synchronization = synchro;
        localThreadData[i]->spiriousWakeup = spiriousWakeup;
        if (0 != (errorCode = pthread_create(&synchro->philosophers[i], NULL, philosopher, (void*)localThreadData[i]))){
            perror("pthread_create error");
                for (int j = 0; j < i; j++){
                    if (0 != pthread_join(synchro->philosophers[i], NULL)){
                        perror("pthread_join after pthread_create error");
                        return EXIT_FAILURE;
                    }
                }

            
            destroySynchronization(synchro);
            free(synchro);

            for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
                free(localThreadData[i]);
            }
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        LocalThreadData* returned;
        if (0!= pthread_join(synchro->philosophers[i], (void*)&returned)){
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
        if (NULL != returned){
            fprintf(stderr, "Bad finish\n");
            return EXIT_FAILURE;
        }
    }

    destroySynchronization(synchro);
    free(synchro);

    for(int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        free(localThreadData[i]);
    }

    return EXIT_SUCCESS;
}
