#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define COUNT_OF_THREADS 2 
#define MAX_LENGTH_OF_STRING 80
#define ERROR -1
#define SUCCESS 0

typedef struct Node{
    char* data;
    struct Node* next;
}Node;

typedef struct SharedData{
    pthread_rwlock_t rwlock;
    int currentListSize;
    struct Node* head;
}SharedData;

void cleanSharedData(SharedData*);
void* sortList(void*);
int readLock(pthread_rwlock_t*);
int writeLock(pthread_rwlock_t*);
int unlock(pthread_rwlock_t*);
void printList(SharedData*);
Node* addFirstElement(SharedData*, char*);
void swap(Node*, Node*);
void destroyList(SharedData*);


int readLock(pthread_rwlock_t* rwlock){
    int errorCode = 0;
    errorCode = pthread_rwlock_rdlock(rwlock);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_rwlock_rdlock error");
        return ERROR;
    }
    return SUCCESS;
}

int writeLock(pthread_rwlock_t* rwlock){
    int errorCode = 0;
    errorCode = pthread_rwlock_wrlock(rwlock);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_rwlock_wrlock error");
        return ERROR;
    }
    return SUCCESS;
}

int unlock(pthread_rwlock_t* rwlock){
    int errorCode = 0;
    errorCode = pthread_rwlock_unlock(rwlock);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_rwlock_unlock error");
        return ERROR;
    }
    return SUCCESS;
}

void cleanSharedData(SharedData* sharedData){
    assert(NULL != sharedData);
    int errorCode = 0;
    if(0 !=(errorCode = pthread_rwlock_destroy(&sharedData->rwlock))){
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
    assert(NULL != string);
    int errorCode = 0;
    Node* newElement = NULL;
    Node* first = sharedData->head;

    if(NULL != first){
        if (SUCCESS != writeLock(&sharedData->rwlock)){
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

        if (SUCCESS != unlock(&sharedData->rwlock)){
            return NULL;
        }

    } else {
        if (SUCCESS != writeLock(&sharedData->rwlock)){
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
        if (SUCCESS != unlock(&sharedData->rwlock)){
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

	    if (SUCCESS != writeLock(&sharedData->rwlock)){
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
         if (SUCCESS != unlock(&sharedData->rwlock)){
            exit(EXIT_FAILURE);
        }

        if (SUCCESS != readLock(&sharedData->rwlock)){
            exit(EXIT_FAILURE);
        }

        printList(sharedData);

        if (SUCCESS != unlock(&sharedData->rwlock)){
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
    int SUCCESS_FINISH = 1; 

    SharedData* sharedData = (SharedData*)malloc(sizeof(SharedData) * COUNT_OF_THREADS);

    if (NULL == sharedData){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    if(0!= (errorCode = pthread_rwlock_init(&sharedData->rwlock, NULL))){
        errno = errorCode;
        perror("pthread_rwlock_init error");
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
            SUCCESS_FINISH = 0;
            break;
        }
        fgets(currentString, MAX_LENGTH_OF_STRING, stdin);
        
        if (0 != ferror(stdin)){
            fprintf(stderr, "An error occurred while reading from the stdin\n");
            SUCCESS_FINISH = 0;
            break;
        }

        if ('\n' != currentString[0]){
            if ('\n' == currentString[strlen(currentString) - 1]) {
                currentString[strlen(currentString) - 1] = '\0';
            }
		    returned = addFirstElement(sharedData, currentString);
            if (NULL == returned){
                SUCCESS_FINISH = 0;
                break;
            }
        }else {

            if (SUCCESS != readLock(&sharedData->rwlock)){
                return EXIT_FAILURE;
            }

            printList(sharedData);

            if (SUCCESS != unlock(&sharedData->rwlock)){
                return EXIT_FAILURE;
            }
        }
    }
    
    
    if (0 != (errorCode = pthread_cancel(sortingThread))){
        errno = errorCode;
        perror("pthread_cancel error");
        return EXIT_FAILURE;
    }

    if (0 != (errorCode = pthread_join(sortingThread, NULL))){
        errno = errorCode;
        perror("pthread_join error");
        return EXIT_FAILURE;
    }
    cleanSharedData(sharedData);
    destroyList(sharedData);

    if (!SUCCESS_FINISH){
        return EXIT_FAILURE;
    } 

    return EXIT_SUCCESS;
}