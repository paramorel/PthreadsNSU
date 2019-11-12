#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define COUNT_OF_ARGUMENTS 2
#define MIN_COUNT_OF_THREADS 1
#define COUNT_OF_STEPS 200000000    
#define CONSTANT_FROM_ROW_LEIBNITZ 4
#define MAX_COUNT_OF_THREADS 3900


typedef struct LocalThreadData {
    int firstElement;
    double partSum;
    long countOfThreads;
} LocalThreadData;

void* calculate(void* parameters){
    assert(NULL != parameters);
    double localPi = 0.0;
    int i=((LocalThreadData*)parameters)->firstElement;
    long countOfThreads = ((LocalThreadData*)parameters)->countOfThreads;

    for (i; i < COUNT_OF_STEPS; i+=countOfThreads){
        localPi += 1.0/(i * 4.0 + 1.0);
		localPi -= 1.0/(i * 4.0 + 3.0);
    }
    
    assert(0 != localPi);

    ((LocalThreadData*)parameters)->partSum = localPi;

	fprintf(stdout, "Thread %d finished. Partial sum %.16f\n", 
			((LocalThreadData*)parameters)->firstElement, localPi);
    
    return parameters;
 }

int main(int argc, char* argv[]){

    if (COUNT_OF_ARGUMENTS != argc){
        fprintf(stderr, "Invalid arguments\n");
        return EXIT_FAILURE;
    }

    int base = 10;
    long countOfThreads = strtol(argv[1], NULL, base);

    if (0 != errno){
        perror("strtol error");
        fprintf(stderr, "Invalid argument. Enter other number\n");
        return EXIT_FAILURE;
    }

    if (MIN_COUNT_OF_THREADS > countOfThreads){
        fprintf(stderr, "You should use at least one thread\n");
        return EXIT_FAILURE;
    }

    if (MAX_COUNT_OF_THREADS < countOfThreads){
        countOfThreads = MAX_COUNT_OF_THREADS;
    }

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
        parameters[i].countOfThreads = countOfThreads;
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
    fprintf(stdout, "You used %ld threads\n", countOfThreads);
    
    if (MAX_COUNT_OF_THREADS == countOfThreads){
        fprintf(stdout, "This is the maximum possible number of threads\n");
    }

    free(parameters);

    return EXIT_SUCCESS;
}
