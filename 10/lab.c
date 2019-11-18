#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define COUNT_OF_LINES_TO_PRINT 10
#define COUNT_OF_MUTEXES 3  

static pthread_mutex_t mutex[2];
static pthread_mutexattr_t mutexAttr;
static int childStarted = 0;

void initMutexes();
void cleanResources();
void* printMessage(void*);
void lockMutex(pthread_mutex_t*);
void unlockMutex(pthread_mutex_t*);


void *printMessage(void *threadData) {
    lockMutex(&mutex[2]);
    int k = 2;
    childStarted = 1;
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&mutex[(k + 1) % COUNT_OF_MUTEXES]);

        fprintf(stdout, "Child thread\n");

        unlockMutex(&mutex[k]);
        k = (k + 1) % COUNT_OF_MUTEXES;
    }

    unlockMutex(&mutex[k]);
    return NULL;
}

void lockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    if (0 != (errorCode = pthread_mutex_lock(mutex))){
        errno = errorCode;
        perror("pthread_mutex_lock error");
        cleanResources();
        exit(EXIT_FAILURE);
    }
}

void unlockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    if (0 != (errorCode = pthread_mutex_unlock(mutex))){
        errno = errorCode;
        perror("pthread_mutex_unlock error");
        cleanResources();
        exit(EXIT_FAILURE);
    }
}

void cleanResources(){
    int errorCode = 0;
    for (int i = 0; i < COUNT_OF_MUTEXES; i++){
        if (0 != (errorCode = pthread_mutex_destroy(&mutex[i]))){
            errno = errorCode;
            perror("pthread_mutex_destroy error");
        }
    }
}

void initMutexes(){
    int errorCode = 0;
    if(0 != (errorCode = pthread_mutexattr_init(&mutexAttr))){
        errno = errorCode;
        perror("pthread_mutexattr_init error");
        exit(EXIT_FAILURE);
    }

    if (0 != (errorCode = pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK))){
        errno = errorCode;
        perror("pthread_mutexattr_settype");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < COUNT_OF_MUTEXES; i++){
        if (0 != (errorCode = pthread_mutex_init(&mutex[i], &mutexAttr))){
            errno = errorCode;
            perror("pthread_mutex_init error");
            for (int j = 0; j < i; j++){
                if (0 != (errorCode = pthread_mutex_destroy(&mutex[i]))){
                    errno = errorCode;
                    perror("pthread_mutex_destroy after init error");
                }
            }
            if (0 != (errorCode = pthread_mutexattr_destroy(&mutexAttr))){
                errno = errorCode;
                perror("pthread_mutexattr_destroy after init error");
            }
            exit(EXIT_FAILURE);
        }
    }
}


int main(int argc, char *argv[]) {
    int errorCode = 0;
    pthread_t thread;

    initMutexes();

    lockMutex(&mutex[0]);
    if (0 != (errorCode = pthread_create(&thread, NULL, printMessage, NULL))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanResources();
        return EXIT_FAILURE;
    }

    while (!childStarted){
        if(0 !=  sleep(0)){
            fprintf(stderr, "defective sleep");
            cleanResources();
            return EXIT_FAILURE;
        }
    }

    int k = 0;

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&mutex[(k + 1) % COUNT_OF_MUTEXES]);

        fprintf(stdout, "Main thread\n");

        unlockMutex(&mutex[k]);
        k = (k + 1) % COUNT_OF_MUTEXES;
    }

    unlockMutex(&mutex[k]);

    if (0 != (errorCode = pthread_join(thread, NULL))){
        errno = errorCode;
        perror("pthread_join error");
        cleanResources();
        return EXIT_FAILURE;
    }

    cleanResources();

    return EXIT_SUCCESS;
}