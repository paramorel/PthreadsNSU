#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#define TIME_TO_SLEEP 2
#define CLEANUP_POP_ARGUMENT 1

void printCleanupText(void* text){
    assert(NULL != text);
    char* cleanupText = (char*)text;
    fprintf(stdout, "%s\n", cleanupText);
}

void* printText(void* threadData) {
    char* cleanupText = "child thread canceled";
    pthread_cleanup_push(printCleanupText, cleanupText);

    while(1){
        pthread_testcancel();
        fprintf(stdout, "Hello world in thread\n");
    }
    pthread_cleanup_pop(CLEANUP_POP_ARGUMENT);
    return NULL;
}

int main(int argc, char* argv[]){
  pthread_t thread;

  if (0 != pthread_create(&thread, NULL, printText, NULL)) {
      perror("pthread_create error");
      return EXIT_FAILURE;
  }

  if (0 != sleep(TIME_TO_SLEEP)) {
    fprintf(stderr, "defective sleep");
    return EXIT_FAILURE;
  }

  if (0 != pthread_cancel(thread)) {
    perror("pthread_cancel error");
    return EXIT_FAILURE;
  }

  if (0 != pthread_join(thread, NULL)){
    perror("pthread_join error");
    return EXIT_FAILURE;
  }

return EXIT_SUCCESS;
}