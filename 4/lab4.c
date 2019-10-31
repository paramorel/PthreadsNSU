#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#define TIME_TO_SLEEP 2

void *printText(void *threadData) {
  while(1){
    pthread_testcancel();
    fprintf(stdout, "Hello world in thread\n");
  }
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