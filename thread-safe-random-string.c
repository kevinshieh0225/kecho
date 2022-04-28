#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_THREAD 10

#define MAX_MSG_LEN 32
#define MIN_MSG_LEN 16
#if MAX_MSG_LEN == MIN_MSG_LEN
#define MASK(num) ((MAX_MSG_LEN - 1))
#elif MIN_MSG_LEN == 0
#define MASK(num) ((num & (MAX_MSG_LEN - 1)))
#else
#define MASK(num) ((num % (MAX_MSG_LEN - MIN_MSG_LEN) + MIN_MSG_LEN))
#endif

static char *rand_string()
{
    int r = MASK(rand());
    char *str = malloc(r + 1);
    str[r] = '\0';
    for (int i = 0; i < r; i++) {
        char c = 97 + rand() % 26;
        str[i] = c;
    }
    return str;
}

static void *print_str(void *str)
{
    printf("%s\n", (char *) str);
    free(str);
}

int main()
{
    srand(time(NULL));
    pthread_t t[MAX_THREAD];
    // unsigned int seed[MAX_THREAD];
    for (int i = 0; i < MAX_THREAD; ++i) {
        char *str = rand_string();
        pthread_create(&t[i], NULL, print_str, str);
    }
    for (int i = 0; i < MAX_THREAD; ++i) {
        pthread_join(t[i], NULL);
    }
    return 0;
}