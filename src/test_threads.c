#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define COUNT 1000

static void* thread1Work(void *p) {
    for (int i = 0; i < COUNT; i++) {
        printf("Thread 1: %d\n", i + 1);
        fflush(stdout);
    }
    pthread_exit(NULL);
    return NULL;
}

static void* thread2Work(void *p) {
    for (int i = 0; i < COUNT; i++) {
        printf("Thread 2: %d\n", i + 1);
        fflush(stdout);
    }
    pthread_exit(NULL);
    return NULL;
}

int main(int argc, char const *argv[]) {
    pthread_t thread1;
    pthread_t thread2;

    if (pthread_create(&thread1, NULL, thread1Work, NULL)) {
        printf("Unable to create thread.\n");
        exit(1);
    }

    if (pthread_create(&thread2, NULL, thread2Work, NULL)) {
        printf("Unable to create thread.\n");
        exit(1);
    }

    pthread_exit(NULL);
    return 0;
}
