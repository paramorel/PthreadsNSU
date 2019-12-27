#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT_OF_THREADS 2 
#define MAX_LENGTH_OF_STRING 80
#define LOCK_OR_UNLOCK_ERROR -1
#define LOCK_OR_UNLOCK_SUCCESS 0

typedef struct Node{
    char* data;
    struct Node* next;
}Node;

typedef struct SharedData{
    pthread_mutex_t mutex;
    int currentListSize;
    struct Node* head;
}SharedData;

void cleanSharedData(SharedData*);
void* sortList(void*);
int lockMutex(pthread_mutex_t*);
int unlockMutex(pthread_mutex_t*);
void printList(SharedData*);
Node* addFirstElement(SharedData*, char*);
void swap(Node*, Node*);
void destroyList(SharedData*);


int lockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_lock(mutex);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_mutex_lock error");
        return LOCK_OR_UNLOCK_ERROR;
    }
    return LOCK_OR_UNLOCK_SUCCESS;
}

int unlockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_unlock(mutex);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_mutex_unlock error");
        return LOCK_OR_UNLOCK_ERROR;
    }
    return LOCK_OR_UNLOCK_SUCCESS;
}

void cleanSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0 !=(errorCode = pthread_mutex_destroy(&sharedData->mutex))){
        errno = errorCode;
        perror("pthread_mutex_destroy error");
    }
}

void destroyList(SharedData* sharedData){
    Node* current = sharedData->head;
    Node* tmp = NULL;
    free(sharedData->head);
    while (current){
	    tmp = current->next;
	    free(current);
	    current = tmp;
    } 
}

void printList(SharedData* sharedData){
    assert(NULL != sharedData);
    Node* currentNode = sharedData->head;
    
    fprintf(stdout, "________Printing_________\n");
    while (currentNode){
	    fprintf (stdout, "%s\n", currentNode->data);
	    currentNode = currentNode->next;
    }
    fprintf(stdout, "________The end__________\n");
    fprintf(stdout, "\n\n");

}

Node* addFirstElement(SharedData* sharedData, char* string){
    assert(NULL != sharedData);
    int errorCode = 0;
    Node* newElement = NULL;
    Node* first = sharedData->head;

    if(NULL != first){
        if (LOCK_OR_UNLOCK_SUCCESS != lockMutex(&sharedData->mutex)){
            return NULL;
        }

        newElement = malloc(sizeof(Node));

        if (NULL == newElement){
            fprintf(stderr, "Memory allocation error\n");
            return NULL;
        }

        sharedData->currentListSize += 1;
        newElement->next = first;
        newElement->data = string;
        sharedData->head = newElement;

        if (LOCK_OR_UNLOCK_SUCCESS != unlockMutex(&sharedData->mutex)){
            return NULL;
        }

    } else {
        if (LOCK_OR_UNLOCK_SUCCESS != lockMutex(&sharedData->mutex)){
            return NULL;
        }

        newElement = malloc(sizeof(Node));

        if (NULL == newElement){
            fprintf(stderr, "Memory allocation error\n");
            return NULL;
        }
        sharedData->currentListSize += 1;
        newElement->next = NULL;
        newElement->data = string;
        sharedData->head = newElement;
        if (LOCK_OR_UNLOCK_SUCCESS != unlockMutex(&sharedData->mutex)){
            return NULL;
        }
    }
    return newElement;
}

void swap(Node* a, Node* b){
    char* tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

void* sortList(void* threadData){
    assert(NULL !=  threadData);
    SharedData* sharedData = (SharedData*)threadData;
    
    while (1){
        Node* list  = (Node*)sharedData->head;
        Node* iteri = NULL;
	    sleep(5);

	    if (LOCK_OR_UNLOCK_SUCCESS != lockMutex(&sharedData->mutex)){
            exit(EXIT_FAILURE);
        }

	    for (iteri = list; iteri; iteri = iteri->next) {
	        Node* iterj = NULL;
	        for (iterj = iteri->next; iterj; iterj = iterj->next) {
	            if (0 < strcmp(iteri->data, iterj->data)) {       
		        swap(iteri, iterj);
		    }
	    }
    }
        
        printList(sharedData);

        if (LOCK_OR_UNLOCK_SUCCESS != unlockMutex(&sharedData->mutex)){
            exit(EXIT_FAILURE);
        }
    }
    
    return NULL;
}


int main(int argc, char* argv[]){
    int errorCode = 0;
    pthread_t sortingThread;
    Node* returned = NULL;
    char* currentString = NULL; 

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    if(0!= (errorCode = pthread_mutex_init(&sharedData->mutex, NULL))){
        errno = errorCode;
        perror("pthread_mutex_init error");
        free(sharedData);
        return EXIT_FAILURE;
    } 

    sharedData->head = NULL;

    if (0 != (errorCode = pthread_create(&sortingThread, NULL, sortList, (void*)sharedData))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanSharedData(sharedData);
        free(sharedData);
        return EXIT_FAILURE;
    }

    while(1){
        currentString = malloc(MAX_LENGTH_OF_STRING * sizeof(char));
        if (NULL == currentString){
            fprintf(stderr, "Memory allocation error\n");
            return EXIT_FAILURE;
        }
        fgets(currentString, MAX_LENGTH_OF_STRING, stdin);
        
        if (0 != ferror(stdin)){
            fprintf(stderr, "An error occurred while reading from the stdin\n");
            return EXIT_FAILURE;
        }

        if ('\n' != currentString[0]){
            if ('\n' == currentString[strlen(currentString) - 1]) {
                currentString[strlen(currentString) - 1] = '\0';
            }
		    returned = addFirstElement(sharedData, currentString);
            if (NULL == returned){
                return EXIT_FAILURE;
            }
        }else {

            if (LOCK_OR_UNLOCK_SUCCESS != lockMutex(&sharedData->mutex)){
                return EXIT_FAILURE;
            }

            printList(sharedData);

            if (LOCK_OR_UNLOCK_SUCCESS != unlockMutex(&sharedData->mutex)){
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}