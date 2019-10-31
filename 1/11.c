#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define COUNT_OF_LINES_TO_PRINT 10

void *printThread(void *thread_data) {
  for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; ++i) {
    fprintf(stdout, "hello world in thread\n");
  }
  sleep(5);
  return NULL;
}


int main(int argc, char *argv[]) {
  void *threadData = NULL;
  pthread_t thread;
    
  for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; ++i) {
    printf("hello world in main thread\n");
  }
 
  if (0 != pthread_create(&thread, NULL, printThread, threadData)) {
    perror("pthread_create error");
    return EXIT_SUCCESS;
  }

  pthread_exit(EXIT_SUCCESS);
  return EXIT_SUCCESS;
}
