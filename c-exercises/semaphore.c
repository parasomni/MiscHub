#define _POSIX_C_SOURCE 199309L
#define MAX_CAPACITY 5

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>
#include <time.h>


int elements_in_list = 0;


typedef struct Node{
    int data;
    struct Node *next;
}node_t;

typedef struct{
    sem_t *mutex;
    sem_t *buffer_empty;
    sem_t *buffer_full;
    node_t *head;
}threadData;


node_t* create_new_node(int data){
    node_t* new_node = malloc(sizeof(node_t));
    new_node->data = data;
    new_node->next = NULL;
    return new_node; 
}

node_t *insert_node_head(node_t *head, int data){
    node_t *node;
    node = create_new_node(data);
    node->next = head;
    return node;
}

node_t* remove_node_tail(node_t* head) {
    if (head == NULL) {
        return head;
    } else if (head->next == NULL) {
        free(head);
        return NULL;
    } else {
        node_t* temp = head;
        while (temp->next->next != NULL) {
            temp = temp->next;
        }
        free(temp->next);
        temp->next = NULL;
        return head;
    }
}

int get_last_element(node_t * head){
    if (head == NULL){
        return 0;
    }
    node_t *helper = head;
    while(helper->next != NULL){
        helper = helper->next;
    }
    return helper->data;
}

int calc_prime_number(int number){
    for(int i = 2; i < number; i++){
        if ((number % i) == 0){
            return 0;
        }
    }
    return number;
}

void stop_clock(struct timespec start,struct timespec end, long *seconds, long *ns){
    clock_gettime(CLOCK_REALTIME, &end);
    *seconds = end.tv_sec - start.tv_sec;
    *ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --*seconds;
        *ns += 1000000000;
    }
}

void *producer(void *args){
    threadData * prodData = (threadData*) args;
    struct timespec start, end;
    long *seconds = malloc(sizeof(long));
    long *ns = malloc(sizeof(long));
    long ns_ran;
    pthread_t threadID = pthread_self();
    int ran_number;
    while(1){
        clock_gettime(CLOCK_REALTIME, &start);
        ns_ran = start.tv_nsec;
        srand(ns_ran);
        ran_number = rand();
        int prime_number = calc_prime_number(ran_number);

        sem_wait(prodData->buffer_empty);
        sem_wait(prodData->mutex);

        prodData->head = insert_node_head(prodData->head, ran_number);
        elements_in_list++;
        printf("[i] Number of elements in list: %d\n", elements_in_list);

        sem_post(prodData->mutex);
        sem_post(prodData->buffer_full);

        stop_clock(start, end, seconds, ns);
        printf("[*] Producer ID %" PRIu64 " done in %ld.%09ld seconds.\n", (uint64_t)threadID, *seconds, *ns);
        sleep(1);
    }
}

void *consumer(void *args){
    threadData *consData = (threadData*) args;
    pthread_t threadID = pthread_self();
    while(1){
        sem_wait(consData->buffer_full);
        sem_wait(consData->mutex);  

        int element = get_last_element(consData->head);
        consData->head = remove_node_tail(consData->head);
        elements_in_list--;
        printf("[i] Number of elements in list: %d\n", elements_in_list);
        printf("[*] Number fetched by consumer ID %" PRIu64 " : %d\n", (uint64_t)threadID, element);

        sem_post(consData->mutex);
        sem_post(consData->buffer_empty);

        sleep(2);
    }
}

int main(){
    printf("[i] Running producer/consumer program.\n");
    int num_producer;
    int num_consumer;
    printf("[A] Please enter a number of producer: ");
    scanf("%d", &num_producer);
    printf("[A] Please enter a number of consumer: ");
    scanf("%d", &num_consumer);

    sem_t mutex;
    sem_t buffer_empty;
    sem_t buffer_full;
    sem_init(&buffer_empty, 0, MAX_CAPACITY);
    sem_init(&buffer_full, 0, 0);
    sem_init(&mutex, 0, 1);

    node_t *head = NULL;
    threadData threadData = {&mutex, &buffer_empty, &buffer_full, head};
    pthread_t *thread_ids = malloc((num_producer + num_consumer) * sizeof(pthread_t));

    for (int i = 0; i < num_producer; i++) {
        pthread_create(&thread_ids[i], NULL, producer, &threadData);
    }
    for (int i = num_producer; i < (num_producer + num_consumer); i++) {
        pthread_create(&thread_ids[i], NULL, consumer, &threadData);
    }
    for (int i = 0; i < (num_producer + num_consumer); i++) {
        pthread_join(thread_ids[i], NULL);
    }

    free(thread_ids);
    sem_destroy(&buffer_empty);
    sem_destroy(&buffer_full);
    sem_destroy(&mutex);
}