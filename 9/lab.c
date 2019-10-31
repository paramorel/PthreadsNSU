#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define COUNT_OF_PHILOSOPHERS 5
#define DELAY 30000
#define FOOD 50

pthread_mutex_t forks[COUNT_OF_PHILOSOPHERS];
pthread_t philosophers[COUNT_OF_PHILOSOPHERS];
pthread_mutex_t foodlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t startTaking = PTHREAD_MUTEX_INITIALIZER;//for second solution

void* philosopher(void* parameter);
int foodOnTable();
void getFork(int philosopher, int fork, char *hand);
void downForks(int f1, int f2);

void* philosopher(void* parameter){
    int id = (int)parameter;
    int leftFork, rightFork, f;

    fprintf(stdout, "Philosopher %d sitting down to dinner.\n", id);
    rightFork = id;
    leftFork = (id + 1) % COUNT_OF_PHILOSOPHERS;

    while (f = foodOnTable()) {
        fprintf (stdout, "Philosopher %d: get dish %d.\n", id, f);
        if (id == 0){//first solution
            getFork(id, leftFork, "left");
            getFork(id, rightFork, "right");
        } else {
            getFork(id, rightFork, "right");
            getFork(id, leftFork, "left");
        }

        /*if (0 != pthread_mutex_lock(&startTaking)){   ///////second solution
            fprintf(stderr, "pthread_mutex_lock error");
            exit(EXIT_FAILURE);
        }

        getFork(id, rightFork, "right");
        getFork(id, leftFork, "left");

        if (0!= pthread_mutex_unlock(&startTaking)){
            fprintf(stderr, "pthread_mutex_unlock error");
            exit(EXIT_FAILURE);
        }*/
        
        fprintf(stdout, "Philosopher %d: eating.\n", id);
        usleep(DELAY * (FOOD - f + 1));
        downForks(leftFork, rightFork);
    }

    fprintf(stdout, "Philosopher %d is done eating.\n", id);
    return NULL;
}

int foodOnTable() {
  static int food = FOOD;
  int myfood;

  if (0 != pthread_mutex_lock(&foodlock)){
      fprintf(stderr, "pthread_mutex_lock error");
      exit(EXIT_FAILURE);
  }

  if (food > 0) {
    food--;
  }

  myfood = food;
  if (0!= pthread_mutex_unlock(&foodlock)){
      fprintf(stderr, "pthread_mutex_unlock error");
      exit(EXIT_FAILURE);
  }
  return myfood;
}

void getFork(int philosopher, int fork, char *hand){
    assert(NULL != hand);
    if (0 != pthread_mutex_lock(&forks[fork])){
        fprintf(stderr, "pthread_mutex_lock error");
        exit(EXIT_FAILURE);
    }
    fprintf (stdout, "Philosopher %d: got %s fork %d\n", philosopher, hand, fork);
}


void downForks(int f1, int f2){
  if (0!= pthread_mutex_unlock(&forks[f1])){
      fprintf(stderr, "pthread_mutex_unlock error");
      exit(EXIT_FAILURE);
  }
  if (0!= pthread_mutex_unlock(&forks[f2])){
      fprintf(stderr, "pthread_mutex_unlock error");
      exit(EXIT_FAILURE);
  }
}

int main(int argc, char* argv[]){

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        if(0 != pthread_mutex_init(&forks[i], NULL)){
            fprintf(stderr, "pthread_mutex_init error");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        if (0 != pthread_create(&philosophers[i], NULL, philosopher, (void *)i)){
            perror("pthread_create error");

            for (int j = 0; j < i; j++){
                if (0!= pthread_join(philosophers[i], NULL)){
                    perror("pthread_join after creating error");
                    return EXIT_FAILURE;
                }
            }

            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        if (0!= pthread_join(philosophers[i], NULL)){
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_PHILOSOPHERS; i++){
        if(0 != pthread_mutex_destroy(&forks[i])){
            fprintf(stderr, "pthread_mutex_destroy error (%d mutex)\n", i);
        }
    }

    return EXIT_SUCCESS;
}