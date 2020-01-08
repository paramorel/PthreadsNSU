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
    int cannotTakeForks;
}LocalThreadData;

void* philosopher(void*);
int initLocalData(LocalThreadData*, Synchronization*);
int initSynchroData(Synchronization*);
void destroySynchronization(Synchronization*);
int lockMutex(pthread_mutex_t*);
int unlockMutex(pthread_mutex_t*);
int foodOnTable(Synchronization*);
int getForks(int, int, LocalThreadData*);
int downForks(int, int, LocalThreadData*);


int lockMutex(pthread_mutex_t* mutex){
    assert(NULL != mutex);
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
    assert(NULL != mutex);
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
    assert(NULL != conditional);
    int errorCode = 0;
    errorCode = pthread_cond_broadcast(conditional);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_cond_broadcast error");
        return FAILURE;
    }
    return SUCCESS;
}

int getForks(int leftFork, int rightFork, LocalThreadData* localThreadData){
    assert(NULL != localThreadData);
    localThreadData->cannotTakeForks = 1;
    int errorCode = 0;
    Synchronization* synchro = localThreadData->synchronization;
    int returned = 0;

    if (SUCCESS != lockMutex(&synchro->startTaking)){
        return FAILURE;
    }
    while(returned){
        returned = pthread_mutex_trylock(&synchro->forks[leftFork]);
        if (returned){
            returned = pthread_mutex_trylock(&synchro->forks[rightFork]);
            if (returned){
                if (SUCCESS != unlockMutex(&synchro->forks[leftFork])){
                    return FAILURE;
                }
            }
        }

        if (returned){
            while (localThreadData->cannotTakeForks){
                if (0 != (errorCode = pthread_cond_wait(&synchro->conditional, &synchro->startTaking))){
                    errno = errorCode;
                    perror("pthread_cond_wait error");
                    return FAILURE;
                }
            } 
        }      
    }

    localThreadData->cannotTakeForks = 1;

    if (SUCCESS != unlockMutex(&synchro->startTaking)){
        return FAILURE;
    }
    return SUCCESS;
}

int downForks(int leftFork, int rightFork, LocalThreadData* localThreadData){
    assert(NULL != localThreadData);
    if (SUCCESS != lockMutex(&localThreadData->synchronization->startTaking)){
        return FAILURE;
    }
    if (SUCCESS != unlockMutex(&localThreadData->synchronization->forks[leftFork])){
        return FAILURE;
    }

    if (SUCCESS != unlockMutex(&localThreadData->synchronization->forks[rightFork])){
        return FAILURE;
    }

    if (SUCCESS != unlockConditional(&localThreadData->synchronization->conditional)){
        return FAILURE;
    }

    localThreadData->cannotTakeForks = 0;

    if (SUCCESS != unlockMutex(&localThreadData->synchronization->startTaking)){
        return FAILURE;
    }

  return SUCCESS;
}

void destroySynchronization(Synchronization* synchronization){
    assert(NULL != synchronization);
    int errorCode = 0;

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        errorCode = pthread_mutex_destroy(&synchronization->forks[i]);
        if (0 != errorCode){
            errno = errorCode;
            perror("pthread_mutex_destroy forks error");
        }
    }

    if (0 != (errorCode = pthread_mutex_destroy(&synchronization->foodlock))){
        errno = errorCode;
        perror("pthread_mutex_destroy foodlock error");
    }

    if (0 != (errorCode = pthread_mutex_destroy(&synchronization->startTaking))){
        errno = errorCode;
        perror("pthread_mutex_destroy starttaking error");
    }

    if (0 != (errorCode = pthread_cond_destroy(&synchronization->conditional))){
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
    int philosopherID = localThreadData->philosopherID;
    Synchronization* synchronization = localThreadData->synchronization;
    int errorCode, leftFork, rightFork;
    int food = 1;

    fprintf(stdout, "Philosopher %d sitting down to dinner.\n", philosopherID);

    rightFork = philosopherID;
    leftFork = philosopherID + 1;
 
    if (leftFork == COUNT_OF_PHILOSOPHERS){
        leftFork = 0;
    }

    while(food){
        food = foodOnTable(synchronization);
        if (food < 0){
            return localThreadData;
        }

        fprintf (stdout, "Philosopher %d: get dish %d.\n", philosopherID, food);
        if (SUCCESS != getForks(leftFork, rightFork, localThreadData)){
            return localThreadData;
        }
        fprintf (stdout, "Philosopher %d: eating.\n", philosopherID);
        usleep(DELAY * (FOOD - food + 1));

        if (SUCCESS != downForks(leftFork, rightFork, localThreadData)){
            return localThreadData;
        }
    }

    fprintf(stdout, "Philosopher %d is done eating.\n", philosopherID);

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
    Synchronization* synchronization;
    int errorCode = 0;

    synchronization = malloc(sizeof(Synchronization));
    if (NULL == synchronization){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    if (SUCCESS != initSynchroData(synchronization)){
        return EXIT_FAILURE;
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        localThreadData[i] = malloc(sizeof(LocalThreadData));   
        if (NULL == localThreadData[i]) {
            fprintf(stderr, "Memory allocation error\n");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        localThreadData[i]->philosopherID = i;
        localThreadData[i]->synchronization = synchronization;
        localThreadData[i]->cannotTakeForks = 0;
        if (0 != (errorCode = pthread_create(&synchronization->philosophers[i], NULL, philosopher, (void*)localThreadData[i]))){
            perror("pthread_create error");
                for (int j = 0; j < i; j++){
                    if (0 != pthread_join(synchronization->philosophers[i], NULL)){
                        perror("pthread_join after pthread_create error");
                        return EXIT_FAILURE;
                    }
                }

            
            destroySynchronization(synchronization);
            free(synchronization);

            for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
                free(localThreadData[i]);
            }
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        LocalThreadData* returned;
        if (0!= pthread_join(synchronization->philosophers[i], (void*)&returned)){
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
        if (NULL != returned){
            fprintf(stderr, "Bad finish\n");
            return EXIT_FAILURE;
        }
    }

    unlockConditional(&synchronization->conditional);
    destroySynchronization(synchronization);
    free(synchronization);

    for(int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        free(localThreadData[i]);
    }

    return EXIT_SUCCESS;
}