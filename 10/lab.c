#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

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
    lockMutex(&mutex[0]);
    childStarted = 1;
    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&mutex[1]);
        unlockMutex(&mutex[0]);
        lockMutex(&mutex[2]);
        unlockMutex(&mutex[1]);

        fprintf(stdout, "Child thread\n");

        lockMutex(&mutex[0]);
        unlockMutex(&mutex[2]);
    }
    unlockMutex(&mutex[0]);
    return NULL;
}

void lockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    if (0 != (errorCode = pthread_mutex_lock(mutex))){
        errno = errorCode;
        perror("pthread_mutex_lock error");
        exit(EXIT_FAILURE);
    }
}

void unlockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    if (0 != (errorCode = pthread_mutex_unlock(mutex))){
        errno = errorCode;
        perror("pthread_mutex_unlock error");
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

    if(0 != (errorCode = pthread_mutexattr_destroy(&mutexAttr))){
        errno = errorCode;
        perror("pthread_mutexattr_init error")
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

    lockMutex(&mutex[1]);
    if (0 != (errorCode = pthread_create(&thread, NULL, printMessage, NULL))) {
        errno = errorCode;
        perror("pthread_create error");
        return EXIT_FAILURE;
    }

    if (!childStarted){
        if(0 != (errorCode = sched_yield())){
            errno = errorCode;
            perror("sched_yield error");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; i++){
        lockMutex(&mutex[2]);
        unlockMutex(&mutex[1]);

        fprintf(stdout, "Main thread\n");

        lockMutex(&mutex[0]);
        unlockMutex(&mutex[2]);
        lockMutex(&mutex[1]);
        unlockMutex(&mutex[0]);
    }

    unlockMutex(&mutex[1]);

    if (0 != (errorCode = pthread_join(thread, NULL))){
        errno = errorCode;
        perror("pthread_join error");
        return EXIT_FAILURE;
    }

    cleanResources();

    return EXIT_SUCCESS;
}