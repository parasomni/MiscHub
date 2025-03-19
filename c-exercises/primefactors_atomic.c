// Summer semester 2024
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdatomic.h>


typedef struct {
    atomic_int *currentNumber;
    int maxNumber;
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

void *calc_prime_numbers(void *args) {
    ThreadData *newData = (ThreadData *) args;
    int currentNumber;
    int primeNumber;
    while ((currentNumber = atomic_fetch_add(newData->currentNumber, 1)) <= newData->maxNumber) {
        if (is_prime(currentNumber)) {
            if (newData->verbose) {
                printf("  %d", currentNumber);
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void stop_clock(struct timespec start,struct timespec end, long *al_seconds, long *al_ns){
    clock_gettime(CLOCK_REALTIME, &end);
    *al_seconds = end.tv_sec - start.tv_sec;
    *al_ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --*al_seconds;
        *al_ns += 1000000000;
    }
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

void run_al1(int maxNumber, int threads,  struct timespec start, struct timespec end, long *al1_seconds, long *al1_ns,  int verbose) {
    atomic_int currentNumber = ATOMIC_VAR_INIT(2);
    ThreadData data = {data.currentNumber = &currentNumber, data.maxNumber = maxNumber, data.verbose = verbose};
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    for (int i = 0; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, calc_prime_numbers, &data);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }
    free(thread_ids);
    stop_clock(start, end, al1_seconds, al1_ns);
    printf("[*] Algorithm [1] done in %ld.%09ld seconds.\n", *al1_seconds, *al1_ns);
}

void run_al2(int number, struct timespec start, struct timespec end, long *al2_seconds, long *al2_ns, int verbose){
    for (int i = 0; i < number; i++){
        if(is_prime(i)){
            if (verbose == 1){
                printf(" %d", i);
                fflush(stdout);
            }   
        }
    }  
    stop_clock(start, end, al2_seconds, al2_ns);
    printf("[*] Algorithm [2] done in %ld.%09ld seconds.\n", *al2_seconds, *al2_ns); 
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

    run_al1(number, threads, start, end, al1_seconds, al1_ns, verbose);
    run_al2(number, start, end, al2_seconds, al2_ns, verbose);

    calc_time_diff(diff_seconds, diff_ns, al1_seconds, al1_ns, al2_seconds, al2_ns);

    free(al1_seconds);
    free(al1_ns);
    free(al2_ns);
    free(al2_seconds);

    return 0;
}
