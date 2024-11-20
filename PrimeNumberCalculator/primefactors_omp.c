// Summer semester 2024
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>
#include <omp.h>

int prime_counter_omp = 0;

int is_prime(int number) {
    if (number <= 1) return 0;
    if (number % 2 == 0 && number != 2) return 0;
    for (int i = 3; i <= sqrt(number); i += 2) {
        if (number % i == 0) return 0;
    }
    return number;
}

void calc_prime_numbers_omp(int startnumber, int maxNumber, int verbose) {
    #pragma omp parallel for reduction(+:prime_counter_omp)
    for (int currentNumber = startnumber; currentNumber <= maxNumber; currentNumber++) {
        if (is_prime(currentNumber)) {
            prime_counter_omp++;
            if (verbose) {
                #pragma omp critical
                {
                    printf("  %d", currentNumber);
                    fflush(stdout);
                }
            }
        }
    }
}

void stop_clock(struct timespec start, struct timespec end, long *al_seconds, long *al_ns) {
    clock_gettime(CLOCK_REALTIME, &end);
    *al_seconds = end.tv_sec - start.tv_sec;
    *al_ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --*al_seconds;
        *al_ns += 1000000000;
    }
}

void run_omp(int startnumber, int maxNumber, int threads, struct timespec start, struct timespec end, int verbose) {
    printf("[i] Running OpenMP algorithm.\r\n");
    long omp_seconds;
    long omp_ns;
    omp_set_num_threads(threads);
    clock_gettime(CLOCK_REALTIME, &start);
    calc_prime_numbers_omp(startnumber, maxNumber, verbose);
    stop_clock(start, end, &omp_seconds, &omp_ns);
    printf("[*] %d prime numbers calculated.\n", prime_counter_omp);
    printf("[*] OpenMP algorithm done in %ld.%09ld seconds.\n", omp_seconds, omp_ns);
}

void run_sequential(int startnumber, int number, struct timespec start, struct timespec end, int verbose) {
    printf("[i] Running sequential algorithm.\r\n");
    int prime_counter_seq = 0;
    long seq_seconds;
    long seq_ns;
    clock_gettime(CLOCK_REALTIME, &start);
    for (int currentNumber = startnumber; currentNumber <= number; currentNumber++) {
        if (is_prime(currentNumber)) {
            prime_counter_seq++;
            if (verbose == 1) {
                printf(" %d", currentNumber);
                fflush(stdout);
            }
        }
    }
    stop_clock(start, end, &seq_seconds, &seq_ns);
    printf("[*] %d prime numbers calculated.\n", prime_counter_seq);
    printf("[*] Sequential algorithm done in %ld.%09ld seconds.\n", seq_seconds, seq_ns);
}

int main() {
    struct timespec start, end;
    int number;
    int threads;
    int verbose;
    int startnumber;

    printf("Please enter a number to calculate all prime numbers: ");
    scanf("%d", &number);
    printf("Please enter start number to count from beginning: ");
    scanf("%d", &startnumber);
    printf("Please enter the number of threads: ");
    scanf("%d", &threads);
    printf("Should prime numbers be printed to stdout? no[0]/yes[1]: ");
    scanf("%d", &verbose);

    clock_gettime(CLOCK_REALTIME, &start);
    run_sequential(startnumber, number, start, end, verbose);
    clock_gettime(CLOCK_REALTIME, &start);
    run_omp(startnumber, number, threads, start, end, verbose);

    return 0;
}
