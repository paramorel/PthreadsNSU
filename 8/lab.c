#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#define COUNT_OF_ARGUMENTS 2
#define MIN_COUNT_OF_THREADS 1
#define COUNT_OF_STEPS 2000000000
#define CONSTANT_FROM_ROW_LEIBNITZ 4

int countOfThreads;
int stopFlag = 0;
long checkInterval = 1000000;
long maxCountSteps = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t barrier;

typedef struct LocalThreadData {
    int firstElement;
    double partSum;
} LocalThreadData;

void checkMutexError(int errorCode, char* comment){
    assert(NULL != comment);
    if (0 != errorCode){
        fprintf(stderr, comment);
        exit(EXIT_FAILURE);
    }
}

void* calculate(void* parameters){
    assert(NULL != parameters);
    LocalThreadData* localData = (LocalThreadData*)parameters;
    int shift = localData->firstElement;
    int lastIterationFlag = 0;
    long nextCheck = checkInterval;
    long i = 0;
    int errorCode;
    double localPi = 0;

    while (!lastIterationFlag && i < COUNT_OF_STEPS){
        if(stopFlag){
            errorCode = pthread_mutex_lock(&mutex);
            checkMutexError(errorCode, "pthread_mutex_lock error");

            if (i > maxCountSteps){
                maxCountSteps = i;
            }

            errorCode = pthread_mutex_unlock(&mutex);
            checkMutexError(errorCode, "pthread_mutex_unlock error");

            lastIterationFlag = 1;
            pthread_barrier_wait(&barrier);

            if (i < maxCountSteps){
                fprintf(stderr, "not enough iterations in thread %d\n", shift);
                nextCheck = maxCountSteps;
            } else {
                fprintf(stdout, "Well done in thread %d\n", shift);
                break;
            }
        }

        for (i; i < nextCheck; i++){
            localPi += 1.0/((i * countOfThreads + shift) * 4.0 + 1.0);
		    localPi -= 1.0/((i * countOfThreads + shift) * 4.0 + 3.0);
        }
        nextCheck += checkInterval;
    }
    
    assert(0 != localPi);
    ((LocalThreadData*)parameters)->partSum = localPi;

	fprintf(stdout, "Thread %d finished. Partial sum %.16f\n", 
			((LocalThreadData*)parameters)->firstElement, localPi);
    
    return parameters;
 }

void signalHandler(int data){
    stopFlag = 1;
    fprintf(stdout, "Stop flag is set\n");
}

int main(int argc, char* argv[]){
    if (COUNT_OF_ARGUMENTS != argc){
        fprintf(stderr, "Invalid arguments");
        return EXIT_FAILURE;
    }

    countOfThreads = atoi(argv[1]);

    if (MIN_COUNT_OF_THREADS > countOfThreads){
        fprintf(stderr, "You should use at least one thread");
        return EXIT_FAILURE;
    }


    if (MAX_COUNT_OF_THREADS < countOfThreads){
        countOfThreads = MAX_COUNT_OF_THREADS;
    }

    if (SIG_ERR == sigset(SIGINT, signalHandler)){
        perror("sigset error");
        return EXIT_FAILURE;
    }

    if (0 != pthread_barrier_init(&barrier, NULL, countOfThreads)){
        fprintf(stderr, "pthread_barrier_init error");
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Press Ctrl+C to stop computing\n");

    LocalThreadData* parameters = malloc(countOfThreads * sizeof(LocalThreadData));

    if (NULL == parameters){
       fprintf(stderr, "memory allocation error");
       return EXIT_FAILURE;
    }

    pthread_t threads[countOfThreads];
    double pi = 0;

    for (int i = 0; i < countOfThreads; i++){
        parameters[i].firstElement = i;
        parameters[i].partSum = 0;
        if (0 != pthread_create(&threads[i], NULL, calculate, (void*)(parameters + i))){
            perror("pthread_create error");

            for (int j = 0; j < i; j++){
                if (0 != pthread_join(threads[i], NULL)){
                    perror("pthread_join after pthread_create error");
                    free(parameters);
                    return EXIT_FAILURE;
                }
            }
            free(parameters);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < countOfThreads; i++){
        LocalThreadData* result;
        if (0 != pthread_join(threads[i], (void*)&result)){
            perror("pthread_join error");
            free(parameters);
            return EXIT_FAILURE;
        }
        pi += result->partSum;
    }

    pi *= CONSTANT_FROM_ROW_LEIBNITZ;
    fprintf(stdout, "%.16f\n", pi);
    fprintf(stdout, "You used %d threads\n", countOfThreads);
    free(parameters);

    if (0 != pthread_barrier_destroy(&barrier)){
        fprintf(stderr, "pthread_barrier_destroy error");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
