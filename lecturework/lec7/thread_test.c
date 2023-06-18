#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
void *thread_proc(void *);
int main() { 
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, thread_proc, NULL)) {
        printf("Failed to create new thread\n");
        return 1;
    }
    pthread_join(thread_id, NULL);
    return 0; 
}
void *thread_proc(void *arg) {
    printf("start\n");
    for (int i = 0; i < 3; i++) {
        printf("Child thread %d\n", i);
        sleep(1);
    }
}

