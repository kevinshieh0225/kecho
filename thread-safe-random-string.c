#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

// #define barrier() __asm__ __volatile__("" : : : "memory")
#define MAX_THREAD 1000
#define BENCH_COUNT 500

#define MAX_MSG_LEN 32
#define MIN_MSG_LEN 16
#if MAX_MSG_LEN == MIN_MSG_LEN
#define MASK(num) ((MAX_MSG_LEN - 1))
#elif MIN_MSG_LEN == 0
#define MASK(num) ((num & (MAX_MSG_LEN - 1)))
#else
#define MASK(num) (((num % (MAX_MSG_LEN - MIN_MSG_LEN)) + MIN_MSG_LEN))
#endif

bool ready;

static pthread_mutex_t worker_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t worker_wait = PTHREAD_COND_INITIALIZER;
static int idx = 0;
pthread_t pt[MAX_THREAD];
int record[BENCH_COUNT];

// static char *rand_string()
// {
//     int r = MASK(rand());
//     char *str = malloc(r + 1);
//     str[r] = '\0';
//     for (int i = 0; i < r; i++) {
//         char c = 97 + rand() % 26;
//         str[i] = c;
//     }
//     return str;
// }

static void *bench_worker(__attribute__((unused)))
{
    /* wait until all workers created */
    pthread_mutex_lock(&worker_lock);
    while (!ready) {
        idx += 1;
        if (pthread_cond_wait(&worker_wait, &worker_lock)) {
            puts("pthread_cond_wait failed");
            exit(-1);
        }
    }
    pthread_mutex_unlock(&worker_lock);

    // printf("%s\n", str);
    // free(str);
    pthread_exit(NULL);
}

static void create_worker(int thread_qty)
{
    // srand(time(NULL));
    for (int i = 0; i < thread_qty; i++) {
        // char *str = rand_string();
        if (pthread_create(&pt[i], NULL, bench_worker, NULL)) {
            puts("thread creation failed");
            exit(-1);
        }
    }
}

int main()
{
    for (int i = 0; i < BENCH_COUNT; ++i) {
        ready = false;
        create_worker(MAX_THREAD);
        pthread_mutex_lock(&worker_lock);
        ready = true;

        /* all workers are ready, let's start bombing kecho */
        pthread_cond_broadcast(&worker_wait);
        pthread_mutex_unlock(&worker_lock);

        /* waiting for all workers to finish the measurement */
        for (int x = 0; x < MAX_THREAD; x++)
            pthread_join(pt[x], NULL);
        record[i] = idx;
        idx = 0;
    }
    for (int i = 0; i < BENCH_COUNT; ++i)
        printf("%d ", record[i]);
    printf("\n");
    return 0;
}