#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define COUNT_OF_LINES_TO_PRINT 10
#define COUNT_OF_MUTEXES 3

pthread_mutex_t mutex[2];
pthread_mutexattr_t mutexAttr;
int childStarted = 0;

void initMutexes();
void cleanResources();
void* printMessage(void*);
void lockMutex(pthread_mutex_t*);
void unlockMutex(pthread_mutex_t*);


void *printMessage(void *threadData) {
    lockMutex(&mutex[0]);
    childStarted = 1;
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&mutex[1]);
        unlockMutex(&mutex[0]);
        lockMutex(&mutex[2]);
        unlockMutex(&mutex[1]);

        fprintf(stdout, "It's a child thread\n");

        lockMutex(&mutex[0]);
        unlockMutex(&mutex[2]);
    }
    unlockMutex(&mutex[0]);
    return NULL;
}

void lockMutex(pthread_mutex_t* mutex){
    if (0 != pthread_mutex_lock(mutex)){
        perror("pthread_mutex_lock error");
        exit(EXIT_FAILURE);
    }
}

void unlockMutex(pthread_mutex_t* mutex){
    if (0 != pthread_mutex_unlock(mutex)){
        perror("pthread_mutex_unlock error");
        exit(EXIT_FAILURE);
    }
}

void cleanResources(){
    if (0 != pthread_mutexattr_destroy(&mutexAttr)){
        perror("pthread_mutexattr_destroy error");
    }

    for (int i = 0; i < COUNT_OF_MUTEXES; i++){
        if (0 != pthread_mutex_destroy(&mutex[i])){
            perror("pthread_mutex_destroy error");
        }
    }

}

void initMutexes(){
    int errorCode = 0;
    if(0 != pthread_mutexattr_init(&mutexAttr)){
        perror("pthread_mutexattr_init error");
        exit(EXIT_FAILURE);
    }

    if (0 != pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK)){
        perror("pthread_mutexattr_settype");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < COUNT_OF_MUTEXES; i++){
        if (0 != pthread_mutex_init(&mutex[i], &mutexAttr)){
            perror("pthread_mutex_init error");
            for (int j = 0; j < i; j++){
                if (0 != pthread_mutex_destroy(&mutex[i])){
                    perror("pthread_mutex_destroy after init error");
                }
            }
            if (0 != pthread_mutexattr_destroy(&mutexAttr)){
                perror("pthread_mutexattr_destroy after init error");
            }
            exit(EXIT_FAILURE);
        }
    }
}


int main(int argc, char *argv[]) {
    pthread_t thread;

    initMutexes();

    lockMutex(&mutex[1]);
    if (0 != pthread_create(&thread, NULL, printMessage, NULL)) {
        perror("pthread_create error");
        return EXIT_FAILURE;
    }

    if (!childStarted){
        if(0 != sched_yield()){
            perror("sched_yield error");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&mutex[2]);
        unlockMutex(&mutex[1]);

        fprintf(stdout, "It's a main thread\n");

        lockMutex(&mutex[0]);
        unlockMutex(&mutex[2]);
        lockMutex(&mutex[1]);
        unlockMutex(&mutex[0]);
    }

    unlockMutex(&mutex[1]);

    pthread_exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}