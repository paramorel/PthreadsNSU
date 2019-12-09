#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT_OF_THREADS 2 
#define MAX_LENGTH_OF_STRING 80

typedef struct Node{
    char* data;
    struct Node* next;
}Node;

typedef struct SharedData{
    pthread_mutex_t mutex;
    int currentListSize;
    struct Node* head;
}SharedData;

void exitBecauseError(int, char*);
void initSharedData(SharedData*);
void cleanSharedData(SharedData*);
void* sortingList(void*);
void lockMutex(pthread_mutex_t*);
void unlockMutex(pthread_mutex_t*);
void printList(SharedData*);
void addFirstElement(SharedData*, char*);
void swap(Node*, Node*);
void destroyList(SharedData*);

void exitBecauseError(int errorCode, char* message){
    if (0 != errorCode){
        if (NULL == message){
            message = "error message ";
        }
        fprintf(stderr, message, strerror(errorCode));
        exit(EXIT_FAILURE);
    }
}

void lockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_lock(mutex);
    exitBecauseError(errorCode, "pthread_mutex_lock error");
}

void unlockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_unlock(mutex);
    exitBecauseError(errorCode, "pthread_mutex_unlock error");
}

void initSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0!= (errorCode = pthread_mutex_init(&sharedData->mutex, NULL))){
        errno = errorCode;
        perror("pthread_mutex_init 1 error");
        free(sharedData);
        exit(EXIT_FAILURE);
    } 

    sharedData->head = NULL;
    sharedData->currentListSize = 0;
}

void cleanSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0 !=(errorCode = pthread_mutex_destroy(&sharedData->mutex))){
        errno = errorCode;
        perror("pthread_mutex_destroy error");
    }
    sharedData->head = NULL;
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
    lockMutex(&sharedData->mutex);
    Node* currentNode = sharedData->head;
    
    fprintf(stdout, "________Printing_________\n");
    while (currentNode){
	    fprintf (stdout, "%s\n", currentNode->data);
	    currentNode = currentNode->next;
    }
    fprintf(stdout, "________The end__________\n");
    fprintf(stdout, "\n");

    unlockMutex(&sharedData->mutex);
}

void addFirstElement(SharedData* sharedData, char* string){
    assert(NULL != sharedData);
    Node* newElement = NULL;
    Node* first = sharedData->head;

    if(NULL != first){
        lockMutex(&sharedData->mutex);
        newElement = malloc(sizeof(Node));

        if (NULL == sharedData){
            fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
        }
        sharedData->currentListSize += 1;
        newElement->next = first;
        newElement->data = string;
        sharedData->head = newElement;
        unlockMutex(&sharedData->mutex);
    } else {
        lockMutex(&sharedData->mutex);
        newElement = malloc(sizeof(Node));

        if (NULL == sharedData){
            fprintf(stderr, "Memory allocation error\n");
            exit(EXIT_FAILURE);
        }
        sharedData->currentListSize += 1;
        newElement->next = NULL;
        newElement->data = string;
        sharedData->head = newElement;
        unlockMutex(&sharedData->mutex);
    }
}

void swap(Node* a, Node* b){
    char* tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

void* sortingList(void* threadData){
    assert(NULL !=  threadData);
    SharedData* sharedData = (SharedData*)threadData;
    
    while (1){
        Node* list  = (Node*)sharedData->head;
        Node* iteri = NULL;
	    sleep(5);

	    lockMutex(&sharedData->mutex);
	    for (iteri = list; iteri; iteri = iteri->next) {
	        Node* iterj = NULL;
	        for (iterj = iteri->next; iterj; iterj = iterj->next) {
	            if (0 < strcmp(iteri->data, iterj->data)) {       
		        swap(iteri, iterj);
		    }
	    }
	}
        unlockMutex(&sharedData->mutex);
        printList(sharedData);
    }
    
    return NULL;
}


int main(int argc, char* argv[]){
    int errorCode = 0;
    int returnFromFgetc = 0;
    pthread_t sortingThread;
    char* currentString = NULL;

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    initSharedData(sharedData);

    if (0 != (errorCode = pthread_create(&sortingThread, NULL, sortingList, (void*)sharedData))) {
        errno = errorCode;
        perror("pthread_create error");
        cleanSharedData(sharedData);
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
		    addFirstElement(sharedData, currentString);
        }else {
            printList(sharedData);
        }
    }

    return EXIT_FAILURE;
}