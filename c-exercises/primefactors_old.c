// Summer semester 2024
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdatomic.h>

typedef struct{
    int startNumber;
    int endNumber;
    int verbose;
} ThreadData;

int is_prime(int number) {
    if (number <= 1) return 0;
    if (number % 2 == 0 && number != 2) return 0;
    for (int i = 3; i <= sqrt(number); i += 2) {
        if (number % i == 0) return 0;
    }
    return number;
}
/*
int calc_prime_number(int number){
    for(int i = 2; i < number; i++){
        if ((number % i) == 0){
            return 0;
        }
    }
    return number;
}
*/

void stop_clock(struct timespec start,struct timespec end, long *al_seconds, long *al_ns){
    clock_gettime(CLOCK_REALTIME, &end);
    *al_seconds = end.tv_sec - start.tv_sec;
    *al_ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --*al_seconds;
        *al_ns += 1000000000;
    }
}

void *calc_prime_numbers(void *args){
    ThreadData * newData = (ThreadData *) args;
    int startNumber = newData->startNumber;
    int endNumber = newData->endNumber;
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " calculating prime numbers [%d - %d]\n", (uint64_t)threadID, newData->startNumber, newData->endNumber);
    for (; startNumber < endNumber; startNumber++){
        if (is_prime(startNumber) == 0){
        }else{
            if(newData->verbose == 1){
                printf("  %d", startNumber);
            }
        }
    }
}

void run_al1(int number, int threads, struct timespec start, struct timespec end, long *al1_seconds, long *al1_ns, int verbose){
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    ThreadData * threadDataArray = malloc(threads * sizeof(ThreadData));
    if (!threadDataArray) {
        fprintf(stderr, "[E] Failed to allocate memory\n");
        exit(1);
    }
    
    // improvement: better block calculation depending on threads
    // higher blocks should be smaller and lower number blocks larger
    printf("[i] Calculating block size for each thread.\n");
    if ((number % threads) == 0){
        for (int i = 0; i < threads; i++){
            threadDataArray[i].startNumber = i * (number / threads);
            threadDataArray[i].endNumber = (i+1) * (number / threads);
        }
        printf("[i] Block size each thread: %d\n", (number / threads));
    }else{
        int rest = number % threads;
        int numberWithoutRest = number - rest;
        for (int i = 0; i < (threads - 1); i++){
            threadDataArray[i].startNumber = i * (numberWithoutRest / threads);
            threadDataArray[i].endNumber = (i+1) * (numberWithoutRest / threads);
        }
        threadDataArray[threads-1].startNumber= (threads-1) * (numberWithoutRest / threads); 
        threadDataArray[threads-1].endNumber = (threads) * (numberWithoutRest / threads) + rest; 
        printf("[i] Block size each thread: %d\n", (numberWithoutRest / threads));
        printf("[i] Block size last thread: %d\n", (numberWithoutRest / threads) + rest);
    }

    pthread_t* threadsID = malloc(threads * sizeof(pthread_t));

    printf("[*] Calculating prime numbers.\n");
    for (int i = 0; i < threads; i++){
        threadDataArray[i].verbose = verbose;
        pthread_create((pthread_t *)&threadsID[i], NULL, calc_prime_numbers, (void *)&threadDataArray[i]);
    }

    for (int i = 0; i < threads; i++) {
        pthread_join(threadsID[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }

    free(threadsID);
    free(threadDataArray);

    stop_clock(start, end, al1_seconds, al1_ns);
    printf("[*] Algorithm [1] done in %ld.%09ld seconds.\n", *al1_seconds, *al1_ns);
} 

void run_al3(int number, struct timespec start, struct timespec end, long *al3_seconds, long *al3_ns, int verbose){
    printf("[i] Algorithm [3] calculating prime numbers 10.000.000 to 10.010.000.\n");
    for (int i = 10000000; i < number; i++)
        if(is_prime(i) == 0){
        }else{
            if (verbose == 1){
                printf(" %d", i);
            }   
        }   
    stop_clock(start, end, al3_seconds, al3_ns);
    printf("[*] Algorithm [3] done in %ld.%09ld seconds.\n", *al3_seconds, *al3_ns); 
}

void run_al2(int number, struct timespec start, struct timespec end, long *al2_seconds, long *al2_ns, int verbose){
    for (int i = 10000000; i < number; i++)
        if(is_prime(i) == 0){
        }else{
            if (verbose == 1){
                printf(" %d", i);
            }   
        }   
    stop_clock(start, end, al2_seconds, al2_ns);
    printf("[*] Algorithm [2] done in %ld.%09ld seconds.\n", *al2_seconds, *al2_ns); 
}

void calc_time_diff(long diff_seconds, long diff_ns, long *al1_seconds, long *al1_ns, long *al2_seconds, long *al2_ns){
    diff_seconds = *al2_seconds - *al1_seconds;
    if (*al1_ns > *al2_ns) {
        diff_seconds--;
        diff_ns = *al1_ns;
        diff_ns += 1000000000;
        diff_ns -= *al2_ns;
        printf("[i] Parallel processing algorithm [1] is %ld.%09ld seconds faster than sequential algorithm [2]. \n\n", diff_seconds, diff_ns); 
    }else{
        diff_ns = *al2_ns - *al1_ns;
        printf("[i] Parallel processing algorithm [1] is %ld.%09ld seconds faster than sequential algorithm [2]. \n\n", diff_seconds, diff_ns); 
    }

}

int main(){
    struct timespec start, end;
    int number;
    int threads;
    int verbose;
    long * al2_seconds = malloc(sizeof(long));
    long * al2_ns = malloc(sizeof(long));
    long * al1_seconds = malloc(sizeof(long));
    long * al1_ns = malloc(sizeof(long));
    long * al3_seconds = malloc(sizeof(long));
    long * al3_ns = malloc(sizeof(long));
    long diff_ns;
    long diff_seconds;

    printf("Please enter a number to calculate all prime numbers: ");
    scanf("%d", &number);
    printf("Please enter the number of threads: ");
    scanf("%d", &threads);
    printf("Should prime numbers be printed to stdout? no[0]/yes[1]: ");
    scanf("%d", &verbose);
    printf("[i] Starting calculations.\n");
    clock_gettime(CLOCK_REALTIME, &start);

    run_al3(10010000, start, end, al3_seconds, al3_ns, verbose);
    run_al1(number, threads, start, end, al1_seconds, al1_ns, verbose);
    run_al2(number, start, end, al2_seconds, al2_ns, verbose);

    calc_time_diff(diff_seconds, diff_ns, al1_seconds, al1_ns, al2_seconds, al2_ns);

    free(al1_seconds);
    free(al1_ns);
    free(al2_ns);
    free(al2_seconds);

    return 0;
}
