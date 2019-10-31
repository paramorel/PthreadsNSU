#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define COUNT_OF_LINES_TO_PRINT 10

void *printThread(void *thread_data) {
  for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; ++i) {
    fprintf(stdout, "Hello world in thread\n");
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  void *threadData = NULL;
  pthread_t thread;

  if (0 != pthread_create(&thread, NULL, printThread, threadData)) {
    perror("pthread_create error");
    return EXIT_FAILURE;
  }

  if (0 != pthread_join(thread, NULL)) {
    perror("pthread_join error");
    return EXIT_FAILURE;
  }

  for (int i = 0; i < COUNT_OF_LINES_TO_PRINT; ++i) {
    fprintf(stdout, "Hello world in main thread\n");
  }

  return EXIT_SUCCESS;
}
