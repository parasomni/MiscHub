// Summer semester 2024
#define _POSIX_C_SOURCE 199309L
#define SEM_NAME "/i_am_sem"
#define SEM_NAME_PRIME "/i_am_prime"

#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdatomic.h>
#include <semaphore.h>
#include <fcntl.h>

const size_t MAX_CNT = 10000000;

pthread_mutex_t mutex;
pthread_mutex_t inc_dec_mutex;

int inc_dec_counter = 0;
int inc_dec_counter_sem = 0;
int i_counter = 0;
int prime_counter_sem = 0;
int prime_counter_mutex = 0;
atomic_int prime_counter_atomic = 0;
int prime_counter_seq = 0;

typedef struct{
int * inc_dec_counter;
int inc_dec;
} IncDecData;

typedef struct {
    atomic_int *currentNumber;
    int maxNumber;
    int verbose;
} ThreadData;

typedef struct {
    int * counter;
    int maxNumber;
    int verbose;
} MutexData;

typedef struct {
    sem_t *sem;
    int * counter;
    int maxNumber;
    int verbose;
} SemData;



int is_prime(int number) {
    if (number <= 1) return 0;
    if (number % 2 == 0 && number != 2) return 0;
    for (int i = 3; i <= sqrt(number); i += 2) {
        if (number % i == 0) return 0;
    }
    return number;
}

void *calc_prime_numbers_atomic(void *args) {
    ThreadData *newData = (ThreadData *) args;
    int currentNumber;
    int primeNumber;
    while ((currentNumber = atomic_fetch_add(newData->currentNumber, 1)) <= newData->maxNumber) {
        if (is_prime(currentNumber)) {
            atomic_fetch_add(&prime_counter_atomic, 1);
            if (newData->verbose) {
                printf("  %d", currentNumber);
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void *calc_prime_numbers_mutex(void *args) {
    MutexData *newData = (MutexData *) args;
    while(1){
        pthread_mutex_lock(&mutex);
        if (*newData->counter > newData->maxNumber){
            pthread_mutex_unlock(&mutex);
            break;
        }
        int currentNumber = *newData->counter;
        (*newData->counter)++;
        pthread_mutex_unlock(&mutex);
        if (is_prime(currentNumber)) {
            pthread_mutex_lock(&mutex);
            prime_counter_mutex++;
            pthread_mutex_unlock(&mutex);
            if (newData->verbose) {
                printf("  %d", currentNumber);
                fflush(stdout);
            }
        }
    }
    return NULL;
}

void *calc_prime_numbers_sem(void *args) {
    SemData *newData = (SemData *) args;
    while(1){
        sem_wait(newData->sem);
        if (*newData->counter > newData->maxNumber){
            sem_post(newData->sem);
            break;
        }
        int currentNumber = *newData->counter;
        (*newData->counter)++;
        sem_post(newData->sem);
        if (is_prime(currentNumber)) {
            sem_wait(newData->sem);
            prime_counter_sem++;
            sem_post(newData->sem);
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

void calc_time_diff(long diff_seconds, long diff_ns, long *al1_seconds, 
        long *al1_ns, long *al2_seconds, long *al2_ns, long *al3_ns, long *al3_seconds, long *sem_seconds, long *sem_ns){
    diff_seconds = *al2_seconds - *al1_seconds;
    if (*al1_ns > *al2_ns && diff_seconds > 0) {
        diff_seconds--;
        diff_ns = *al1_ns;
        diff_ns += 1000000000;
        diff_ns -= *al2_ns;
        printf("[i] Parallel processing algorithm is %ld.%09ld seconds faster than sequential algorithm. \n", diff_seconds, diff_ns); 
    }else{
        diff_ns = *al2_ns - *al1_ns;
        printf("[i] Parallel processing algorithm is %ld.%09ld seconds faster than sequential algorithm. \n", diff_seconds, diff_ns); 
    }
    diff_seconds = *al1_seconds - *al3_seconds;
    if (*al3_ns > *al1_ns && diff_seconds > 0) {
        diff_seconds--;
        diff_ns = *al3_ns;
        diff_ns += 1000000000;
        diff_ns -= *al1_ns;
        if(*al1_seconds > *al3_seconds || (*al1_seconds >= *al3_seconds ) && *al3_ns < *al1_ns){
            printf("[i] Mutex algorithm is %ld.%09ld seconds faster than atomic algorithm. \n", diff_seconds, diff_ns); 
        }else{
            printf("[i] Mutex algorithm is %ld.%09ld seconds slower than atomic algorithm. \n", diff_seconds, diff_ns); 
        }
    }else{
        diff_ns = *al1_ns - *al3_ns;
        if(*al1_seconds > *al3_seconds || (*al1_seconds >= *al3_seconds ) && *al3_ns < *al1_ns){
            printf("[i] Mutex algorithm is %ld.%09ld seconds faster than atomic algorithm. \n", diff_seconds, diff_ns); 
        }else{
            printf("[i] Mutex algorithm is %ld.%09ld seconds faster than atomic algorithm. \n", diff_seconds, diff_ns); 
        }
    }
    diff_seconds = *sem_seconds - *al3_seconds;
    if (*sem_ns > *al3_ns && diff_seconds > 0) {
        diff_seconds--;
        diff_ns = *sem_ns;
        diff_ns += 1000000000;
        diff_ns -= *al3_ns;
        if(*al3_seconds > *sem_seconds || (*al3_seconds >= *sem_seconds ) && *sem_ns < *al3_ns){
            printf("[i] Semaphore algorithm is %ld.%09ld seconds faster than mutex algorithm. \n\n", diff_seconds, diff_ns); 
        }else{
            printf("[i] Semaphore algorithm is %ld.%09ld seconds slower than mutex algorithm. \n\n", diff_seconds, diff_ns); 
        }
    }else{
        diff_ns = *sem_ns - *al3_ns;
        if(*al3_seconds > *sem_seconds || (*al3_seconds >= *sem_seconds ) && *sem_ns < *al3_ns){
            printf("[i] Semaphore algorithm is %ld.%09ld seconds faster than mutex algorithm. \n\n", diff_seconds, diff_ns); 
        }else{
            printf("[i] Semaphore algorithm is %ld.%09ld seconds slower than mutex algorithm. \n\n", diff_seconds, diff_ns); 
        }
    }

}

void run_atomic(int startnumber, int maxNumber, int threads,  struct timespec start, struct timespec end, long *al1_seconds, long *al1_ns,  int verbose) {
    printf("[i] Running atomic algorithm.\r\n");
    atomic_int currentNumber = ATOMIC_VAR_INIT(startnumber);
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    ThreadData data = {&currentNumber, maxNumber, verbose};
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    for (int i = 0; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, calc_prime_numbers_atomic, &data);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    free(thread_ids);
    stop_clock(start, end, al1_seconds, al1_ns);
    printf("[*] %d prime numbers calculated.\n", prime_counter_atomic);
    printf("[*] Atomic algorithm done in %ld.%09ld seconds.\n", *al1_seconds, *al1_ns);
}

void run_mutex(int startnumber, int maxNumber, int threads,  struct timespec start, struct timespec end, long *al1_seconds, long *al1_ns,  int verbose) {
    printf("[i] Running mutex algorithm.\r\n");
    int counter = startnumber;
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    MutexData data = {&counter, maxNumber, verbose};
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, calc_prime_numbers_mutex, &data);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    free(thread_ids);
    pthread_mutex_destroy(&mutex);
    stop_clock(start, end, al1_seconds, al1_ns);
    printf("[*] %d prime numbers calculated.\n", prime_counter_mutex);
    printf("[*] Mutex algorithm done in %ld.%09ld seconds.\n", *al1_seconds, *al1_ns);
}

void run_sem(int startnumber, int maxNumber, int threads,  struct timespec start, struct timespec end, long *al1_seconds, long *al1_ns,  int verbose) {
    printf("[i] Running semaphore algorithm.\r\n");
    sem_t *sem =  sem_open(SEM_NAME_PRIME, O_CREAT, 0644, 1);
    int counter = startnumber;
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    SemData data = {sem, &counter, maxNumber, verbose};
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    for (int i = 0; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, calc_prime_numbers_sem, &data);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    free(thread_ids);
    stop_clock(start, end, al1_seconds, al1_ns);
    sem_close(sem);
    sem_unlink(SEM_NAME_PRIME);
    printf("[*] %d prime numbers calculated.\n", prime_counter_sem);
    printf("[*] Semaphore algorithm done in %ld.%09ld seconds.\n", *al1_seconds, *al1_ns);
}

void run_sequential(int startnumber, int number, struct timespec start, struct timespec end, long *al2_seconds, long *al2_ns, int verbose){
    printf("[i] Running sequential algorithm.\r\n");
    for (int i = 0; i < number; i++){
        if(is_prime(i)){
            prime_counter_seq++;
            if (verbose == 1){
                printf(" %d", i);
                fflush(stdout);
            }   
        }
    }  
    stop_clock(start, end, al2_seconds, al2_ns);
    printf("[*] %d prime numbers calculated.\n", prime_counter_seq);
    printf("[*] Sequential algorithm done in %ld.%09ld seconds.\n", *al2_seconds, *al2_ns); 
}

void *inc_var(void *args) {
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " incrementing counter.\r\n", threadID);
    for (int i = 0; i < MAX_CNT; ++i) {
        pthread_mutex_lock(&inc_dec_mutex);
        inc_dec_counter++;
        pthread_mutex_unlock(&inc_dec_mutex);
    }
    return NULL;
}

void *dec_var(void *args) {
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " decrementing counter.\r\n", threadID);
    for (int i = 0; i < MAX_CNT; ++i) {
        pthread_mutex_lock(&inc_dec_mutex);
        inc_dec_counter--;
        //printf("[i] Current number: %d\n", inc_dec_counter);
        pthread_mutex_unlock(&inc_dec_mutex);
    }
    return NULL;
}

void inc_dec_mutexth(struct timespec start, struct timespec end, int threads, int verbose){
    printf("[i] Running increment/decrement algorithm secured by mutex.\r\n");
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    pthread_mutex_init(&inc_dec_mutex, NULL);
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    int num_threads_each_fct = threads / 2;  
    for (int i = 0; i < num_threads_each_fct; i++) {
        pthread_create(&thread_ids[i], NULL, inc_var, NULL);
    }
    for (int i = num_threads_each_fct; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, dec_var, NULL);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    pthread_mutex_destroy(&inc_dec_mutex);
    free(thread_ids);
    printf("[*] Calculated number: %d\r\n", inc_dec_counter);
}

void *inc_atomic(void * args){
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " incrementing counter.\r\n", threadID);
    atomic_int * counter = (atomic_int*) args;
    for (int i = 0; i < MAX_CNT; ++i){
        atomic_fetch_add(counter, 1);
    }
    return NULL;
}

void *dec_atomic(void * args){
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " decrementing counter.\r\n", threadID);
    atomic_int * counter = (atomic_int*) args;
    for (int i = 0; i < MAX_CNT; ++i){
        atomic_fetch_sub(counter, 1);
    }
    return NULL;
}

void *inc_sem(void * args){
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " incrementing counter.\r\n", threadID);
    sem_t *sem = args;
    for (int i = 0; i < MAX_CNT; ++i){
        sem_wait(sem);
        inc_dec_counter_sem++;
        sem_post(sem);
    }
    return NULL;
}

void *dec_sem(void *args){
    pthread_t threadID = pthread_self();
    printf("[*] Thread ID: %" PRIu64 " decrementing counter.\r\n", threadID);
    sem_t *sem = args;
    for(int i = 0; i < MAX_CNT; i++){
        sem_wait(sem);
        inc_dec_counter_sem--;
        sem_post(sem);
    }
    return NULL;
}

void inc_dec_sem(struct timespec start, struct timespec end, int threads, int verbose){
    printf("[i] Running increment/decrement algorithm secured by semaphore.\r\n");
    sem_t *sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    int num_threads_each_fct = threads / 2;  
    for (int i = 0; i < num_threads_each_fct; i++) {
        pthread_create(&thread_ids[i], NULL, inc_sem, sem);
    }
    for (int i = num_threads_each_fct; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, dec_sem, sem);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    free(thread_ids);
    sem_close(sem);
    sem_unlink(SEM_NAME);
    printf("[*] Calculated number: %d\r\n", inc_dec_counter);   
}

void inc_dec_atomic(struct timespec start, struct timespec end, int threads, int verbose){
    printf("[i] Running increment/decrement algorithm secured by atomic.\r\n");
    atomic_int counter = ATOMIC_VAR_INIT(0);
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    int num_threads_each_fct = threads / 2;  
    for (int i = 0; i < num_threads_each_fct; i++) {
        pthread_create(&thread_ids[i], NULL, inc_atomic, &counter);
    }
    for (int i = num_threads_each_fct; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, dec_atomic, &counter);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    free(thread_ids);
    printf("[*] Calculated number: %d\r\n", inc_dec_counter);   
}

void *inc(void *args){
    for (int i = 0; i < MAX_CNT; ++i){
        i_counter++;
    }
}

void *dec(void *args){
    for(int i = 0; i < MAX_CNT; ++i){
        i_counter--;
    }
}

void inc_dec(struct timespec start, struct timespec end, int threads){
    printf("[i] Running unsecured increment/decrement algorithm.\n");
    long * endTimeSec = malloc(sizeof(long));
    long * endTimeNs = malloc(sizeof(long));
    pthread_t *thread_ids = malloc(threads * sizeof(pthread_t));
    int num_threads_each_fct = threads / 2;  
    for (int i = 0; i < num_threads_each_fct; i++) {
        pthread_create(&thread_ids[i], NULL, inc, NULL);
    }
    for (int i = num_threads_each_fct; i < threads; i++) {
        pthread_create(&thread_ids[i], NULL, dec, NULL);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(thread_ids[i], NULL);
        stop_clock(start, end, endTimeSec, endTimeNs);
        printf("[*] Thread [%d] done in %ld.%09ld seconds.\n", i, *endTimeSec, *endTimeNs);
    }
    free(thread_ids);
    printf("[*] Calculated number: %d\r\n", i_counter);
}

int main(){
    struct timespec start, end;
    int number;
    int threads;
    int verbose;
    int startnumber;
    long * al2_seconds = malloc(sizeof(long));
    long * al2_ns = malloc(sizeof(long));
    long * al1_seconds = malloc(sizeof(long));
    long * al1_ns = malloc(sizeof(long));
    long * al3_seconds = malloc(sizeof(long));
    long * al3_ns = malloc(sizeof(long));
    long * sem_seconds = malloc(sizeof(long));
    long * sem_ns = malloc(sizeof(long));
    long diff_ns;
    long diff_seconds;

    printf("Please enter a number to calculate all prime numbers: ");
    scanf("%d", &number);
    printf("Please enter start number to count from beginning: ");
    scanf("%d", &startnumber);
    printf("Please enter the number of threads: ");
    scanf("%d", &threads);
    printf("Should prime numbers be printed to stdout? no[0]/yes[1]: ");
    scanf("%d", &verbose);

    clock_gettime(CLOCK_REALTIME, &start);

    //inc_dec(start, end, threads);
    clock_gettime(CLOCK_REALTIME, &start);

    //inc_dec_atomic(start, end, threads, verbose);
    clock_gettime(CLOCK_REALTIME, &start);

    //inc_dec_mutexth(start, end, threads, verbose);
    clock_gettime(CLOCK_REALTIME, &start);

    //inc_dec_sem(start, end, threads, verbose);
    clock_gettime(CLOCK_REALTIME, &start);

    run_sequential(startnumber, number, start, end, al2_seconds, al2_ns, verbose);
    clock_gettime(CLOCK_REALTIME, &start);

    run_atomic(startnumber, number, threads, start, end, al1_seconds, al1_ns, verbose);
    clock_gettime(CLOCK_REALTIME, &start);
    
    run_mutex(startnumber, number, threads, start, end, al3_seconds, al3_ns, verbose);
    clock_gettime(CLOCK_REALTIME, &start);

    //run_sem(startnumber, number, threads, start, end, sem_seconds, sem_ns, verbose);

    //calc_time_diff(diff_seconds, diff_ns, al1_seconds, 
    //        al1_ns, al2_seconds, al2_ns, al3_ns, al3_seconds, sem_seconds, sem_ns);

    free(al1_seconds);
    free(al1_ns);
    free(al2_ns);
    free(al2_seconds);
    free(al3_ns);
    free(al3_seconds);
    free(sem_seconds);
    free(sem_ns);

    return 0;
}
