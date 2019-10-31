#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define COUNT_OF_THREADS 4
#define COUNT_OF_STRINGS_TO_PRINT 4

void* printStrings(void* lines){
    char** stringsToPrint = (char**)lines;
    for (int i = 0; i < COUNT_OF_STRINGS_TO_PRINT; i++){
        fprintf(stdout, "%s\n", stringsToPrint[i]);
    }
    return NULL;
}

int main(int argc, char* argv[]){

    pthread_t threads[COUNT_OF_THREADS];
    char* stringsToPrint[COUNT_OF_THREADS][COUNT_OF_STRINGS_TO_PRINT] = {
                                    {"thread 1, string 1", "thread 1, string 2", "thread 1, string 3", "thread 1, string 4"},
                                    {"thread 2, string 1", "thread 2, string 2", "thread 2, string 3", "thread 2, string 4"},
                                    {"thread 3, string 1", "thread 3, string 2", "thread 3, string 3", "thread 3, string 4"},
                                    {"thread 4, string 1", "thread 4, string 2", "thread 4, string 3", "thread 4, string 4"}
                                    };

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        if (0 != pthread_create(&threads[i], NULL, printStrings, (void*)stringsToPrint[i])){
            perror("pthread_create error");

            for (int j = 0; j < i; j++){
                if (0 != pthread_join(threads[j], NULL)){
                    perror("pthread_join error");
                    return EXIT_FAILURE;
                }
            }
            return EXIT_FAILURE;            
        }
    }

    for (int i = 0; i < COUNT_OF_THREADS; i++){
        if (0 != pthread_join(threads[i], NULL)){
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
    }
     
    return EXIT_SUCCESS;
}