#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <pthread.h>

static long num_steps = 100000;

void *thread_proc(void *);

int main(){
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, thread_proc, NULL)){
        printf("Failed to create new thread\n");
        return 1;
    }
    pthread_join(thread_id, NULL);
    int i;
    double x, sum =0.0;

    double step = 1.0 / (double)num_steps;
    for (i = 0; i < num_steps; i++){
        x = (i+0.5)*step;
        sum += 4.0/(1.0+x*x);
    }

    double pi = step * sum;
    printf ("PI=%.10f\n", pi);
}