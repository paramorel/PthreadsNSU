#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define COUNT_OF_LINES_TO_PRINT 10

void *printMessage(void *threadData) {
  for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; ++i) {
    fprintf(stdout, "hello world in thread\n");
  }
  sleep(5);
  return NULL;
}


int main(int argc, char *argv[]) {
  pthread_t thread;
    
  for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; ++i) {
    printf("hello world in main thread\n");
  }
 
  if (0 != pthread_create(&thread, NULL, printMessage, NULL)) {
    perror("pthread_create error");
    return EXIT_FAILURE;
  }

  pthread_exit(EXIT_SUCCESS);
  return EXIT_SUCCESS;
}
