#define _POSIX_C_SOURCE 199309L
#define ROCK 0
#define PAPER 1
#define SCISSOR 2

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
#include <unistd.h>

typedef struct{
    int *result_thread;
    sem_t *game;
    sem_t *res;
    int threadID;
}rpsData;

typedef struct{
    int *result_thread_1;
    int * result_thread_2;
    sem_t *game;
    sem_t *res1;
    sem_t *res2;
}gmData;

int game_done = 0;
int game_start = 0;

void print_number(int number, int threadID){
    switch(number){
        case ROCK:
            printf("Thread %d choosed rock.\n", threadID);
            break;
        case PAPER:
            printf("Thread %d choosed paper.\n", threadID);
            break;
        case SCISSOR:
            printf("Thread %d choosed scissor.\n", threadID);
            break;
    }
}

void *rps(void *args){
    rpsData *newData = args;
    struct timespec start, end;
    long ns;
    clock_gettime(CLOCK_REALTIME, &start);
    ns = start.tv_nsec;
    srand(ns);
    while(game_done == 0){
        if (game_start > 0){
            sem_wait(newData->game);
            sem_wait(newData->res);

            *newData->result_thread = rand() % 3;
            int number = *newData->result_thread;

            sem_post(newData->res);
            print_number(number, newData->threadID);
            sem_post(newData->game);

            sleep(1);
        }
    }
}

void *game_master(void *args){
    gmData *newData = args;
    sem_wait(newData->game);
    int rps_counter = 0;
    int rs1;
    int rs2;
    while(rps_counter < 50){
        for(int i = 3; i > 0; i--){
            printf("Rock paper scissors in %d\n", i);
            sleep(1);
        }
        game_start = 1;

        sem_post(newData->game);
        sleep(0.5);
        sem_wait(newData->game);

        game_start = 0;

        sem_wait(newData->res1);
        rs1 = *newData->result_thread_1;
        sem_post(newData->res1);

        sem_wait(newData->res2);
        rs2 = *newData->result_thread_2;
        sem_post(newData->res2);

        rps_counter++;

        if((rs1 == ROCK && rs2 == SCISSOR) || (rs1 == PAPER && rs2 == ROCK) || (rs1 == SCISSOR && rs2 == PAPER)){
            printf("Thread 1 won!\n");
            break;
        }else if((rs2 == ROCK && rs1 == SCISSOR) || (rs2 == PAPER && rs1 == ROCK) || (rs2 == SCISSOR && rs1 == PAPER)){
            printf("Thread 2 won!\n");
            break;
        }else if(rps_counter == 50){
            printf("It's a draw\n");
        }else{
            printf("We need another round!\n");
        }
    }
    game_done = 1;
    sem_post(newData->game);
}

int main(){
    int *result_thread_1 = malloc(sizeof(int));
    int *result_thread_2 = malloc(sizeof(int));

    sem_t res1;
    sem_t res2;
    sem_t game;
    sem_init(&res1, 0, 1);
    sem_init(&res2, 0, 1);
    sem_init(&game, 0, 1);

    gmData gameMaster = {result_thread_1, result_thread_2, &game, &res1, &res2};
    rpsData threadOneData = {result_thread_1, &game, &res1, 1};
    rpsData threadTwoData = {result_thread_2, &game, &res2, 2};

    pthread_t threadOne;
    pthread_t threadTwo;
    pthread_t masterThread;
    pthread_create(&masterThread, NULL, game_master, &gameMaster);
    pthread_create(&threadOne, NULL, rps, &threadOneData);
    pthread_create(&threadTwo, NULL, rps, &threadTwoData);
    pthread_join(masterThread, NULL);
    pthread_join(threadOne, NULL);
    pthread_join(threadTwo, NULL);

    sem_destroy(&res1);
    sem_destroy(&res2);
    sem_destroy(&game);
    
    free(result_thread_1);
    free(result_thread_2);
}