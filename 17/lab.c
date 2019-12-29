#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define MAX_LENGTH_OF_STRING 80
#define ERROR -1
#define SUCCESS 0
#define TIME_TO_SLEEP 5
#define LOCK_OR_UNLOCK_ERROR -2
#define COUNT_OF_SORTING_THREADS 2

typedef struct Node{
    char* data;
    struct Node* next;
    pthread_mutex_t mutex;
}Node;

typedef struct ThreadInfo{
    Node* node;
    int threadID;
}ThreadInfo;

void* sortList(void*);
int lockMutex(pthread_mutex_t*);
int unlockMutex(pthread_mutex_t*);
int printList(Node*);
int addFirstElement(Node*, char*);
void swap(Node*, Node*);
void destroyList(Node*);
int initList(Node*);


int lockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_lock(mutex);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_mutex_lock error");
        return LOCK_OR_UNLOCK_ERROR;
    }
    return SUCCESS;
}

int unlockMutex(pthread_mutex_t* mutex){
    int errorCode = 0;
    errorCode = pthread_mutex_unlock(mutex);
    if (0 != errorCode){
        errno = errorCode;
        perror("pthread_mutex_unlock error");
        return LOCK_OR_UNLOCK_ERROR;
    }
    return SUCCESS;
}


void destroyList(Node* head){
    assert(NULL != head);
    int errorCode = 0;

    if (0 != (errorCode = pthread_mutex_destroy(&head->mutex))){
        errno = errorCode;
        perror("pthread_mutex_destroy 1 error");
    }
    Node* node = head->next;
    while (NULL != node){
        head->next = node->next;
        free(node->data);
        if (0 != (errorCode = pthread_mutex_destroy(&node->mutex))){
            errno = errorCode;
            perror("pthread_mutex_destroy 2 error");
        }
        free(node);
        node = head->next;
    }
}

int initList(Node* head){
    assert(NULL != head);
    int errorCode = 0;

    head->next = NULL;

    if (0 != (errorCode = pthread_mutex_init(&head->mutex, NULL))){
        errno = errorCode;
        perror("pthread_mutex_init error");
        return ERROR;
    }
    return SUCCESS;
}

int printList(Node* head){
    assert(NULL != head);
    int errorCode = 0;

    if (SUCCESS != lockMutex(&head->mutex)){
        return LOCK_OR_UNLOCK_ERROR;
    }

    Node* node;
    Node* prevNode;

    fprintf(stdout, "________Printing_________\n");
    node = head->next;

    if (NULL != node){
        if (SUCCESS != lockMutex(&node->mutex)){
            return LOCK_OR_UNLOCK_ERROR;
        }
    }
     
    if (SUCCESS != unlockMutex(&head->mutex)){
        return LOCK_OR_UNLOCK_ERROR;
    }

    while(NULL != node){
        fprintf(stdout, "%s\n", node->data);
        prevNode = node;
        node = node->next;
        if (NULL != node){
            if (SUCCESS != lockMutex(&node->mutex)){
                return LOCK_OR_UNLOCK_ERROR;
            }
        }

        if (SUCCESS != unlockMutex(&prevNode->mutex)){
            return LOCK_OR_UNLOCK_ERROR;
        }
    }
    fprintf(stdout, "________The end__________\n");
    fprintf(stdout, "\n\n");

    return SUCCESS;
}

int addFirstElement(Node* head, char* string){
    assert(NULL != head);
    assert(NULL != string);
    int errorCode = 0;

    if (SUCCESS != lockMutex(&head->mutex)){
        return LOCK_OR_UNLOCK_ERROR;
    }

    Node* tmp = head->next;
    head->next = malloc(sizeof(Node));

    if (NULL == head->next){
        fprintf(stderr, "Memory allocation error in addFirstElement\n");
        return ERROR;
    }

    if (0 != (errorCode = pthread_mutex_init(&head->next->mutex, NULL))){
        errno = errorCode;
        perror("pthread_mutex_init new error");
        return ERROR;
    }

    head->next->data = string;
    head->next->next = tmp;

    if (SUCCESS != unlockMutex(&head->mutex)){
        return LOCK_OR_UNLOCK_ERROR;
    }

    return SUCCESS;
}

void swap(Node* a, Node* b){
    char* tmp = a->data;
    a->data = b->data;
    b->data = tmp;
}

void* sortList(void* threadData){
    assert(NULL !=  threadData);
    ThreadInfo* threadInfo = (ThreadInfo*)threadData;

    Node* head = threadInfo->node;
    Node *first, *second, *third;
    int errorCode = 0;
    int notSorted;
    while (1) {
	    sleep(TIME_TO_SLEEP);
	    notSorted = 1;
	    while (notSorted) {
	        notSorted = 0;
	        first = head;
            if (SUCCESS != lockMutex(&first->mutex)){
                exit(EXIT_FAILURE);
            }
	        second = head->next;
	        if (NULL != second) { 
                if (SUCCESS != lockMutex(&second->mutex)){
                    exit(EXIT_FAILURE);
                }  
	            third = second->next;
	            while (NULL != third) {
                    if (SUCCESS != lockMutex(&third->mutex)){
                        exit(EXIT_FAILURE);
                    }
	    	        if (0 > strcmp(third->data, second->data)) {
			            notSorted = 1;
	    		        second->next = third->next;
			            first->next = third;
			            third->next = second;
			            second = third;
			            third = second->next;
		            }
                    if (SUCCESS != unlockMutex(&first->mutex)){
                        exit(EXIT_FAILURE);
                    }
		            first = second;
		            second = third;
		            third = third->next;
		        }
                if (SUCCESS != unlockMutex(&second->mutex)){
                    exit(EXIT_FAILURE);
                }
	        }
            if (SUCCESS != unlockMutex(&first->mutex)){
                exit(EXIT_FAILURE);
            }
	    }
        if (0 == threadInfo->threadID){
            if (SUCCESS != printList(head)){
                exit(EXIT_FAILURE);
            }
        }
    }
    return NULL;
}


int main(int argc, char* argv[]){
    int errorCode = 0;
    pthread_t sortingThread[COUNT_OF_SORTING_THREADS];
    ThreadInfo* threadInfo[COUNT_OF_SORTING_THREADS];
    int returned = 0;
    char* currentString = NULL; 
    int SUCCESS_FINISH = 1;

    Node* list = malloc(sizeof(Node));
    if (NULL == list){
        fprintf(stderr, "Memory allocation error\n");
        return EXIT_FAILURE;
    }

    if (SUCCESS != initList(list)){
        return EXIT_FAILURE;
    }

    for (int i = 0; i < COUNT_OF_SORTING_THREADS; i++){
        threadInfo[i] = malloc(sizeof(ThreadInfo));
        if (NULL == threadInfo){
            fprintf(stderr, "Memory allocation error\n");
            destroyList(list);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < COUNT_OF_SORTING_THREADS; i++){
        threadInfo[i]->threadID = i;
        threadInfo[i]->node = list;
        if (0 != (errorCode = pthread_create(&sortingThread[i], NULL, sortList, (void*)threadInfo[i]))) {
            errno = errorCode;
            perror("pthread_create error");
            for (int j = 0; j < i; j++){
                if (0 != (errorCode = pthread_cancel(sortingThread[i]))){
                    errno = errorCode;
                    perror("pthread_cancel error");
                    return EXIT_FAILURE;
                }
                if(0 != (errorCode = pthread_join(sortingThread[i], NULL))){
                    errno = errorCode;
                    perror("pthread_join error");
                    return EXIT_FAILURE;
                }
            }

            destroyList(list);
            return EXIT_FAILURE;
        }
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
		    returned = addFirstElement(list, currentString);
            if (SUCCESS != returned){
                SUCCESS_FINISH = 0;
                if (LOCK_OR_UNLOCK_ERROR == returned){
                    return EXIT_FAILURE;
                }
                break;
            }
        }else {
            returned = printList(list);
            if(SUCCESS != returned){
                SUCCESS_FINISH = 0;
                if (LOCK_OR_UNLOCK_ERROR == returned){
                    return EXIT_FAILURE;
                }
                break;
            }
        }
    }
    
    for (int i = 0; i < COUNT_OF_SORTING_THREADS; i++){
        if (0 != (errorCode = pthread_cancel(sortingThread[i]))){
            errno = errorCode;
            perror("pthread_cancel error");
            return EXIT_FAILURE;
        }
        if (0 != (errorCode = pthread_join(sortingThread[i], NULL))){
            errno = errorCode;
            perror("pthread_join error");
            return EXIT_FAILURE;
        }
    }

    destroyList(list);
    
    if (!SUCCESS_FINISH){
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}