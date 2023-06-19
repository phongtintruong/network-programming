#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <time.h>
#include <pthread.h>

void *client_thread(void *arg);

int main(int argc, char *argv[]){
    if (argc != 3){
        printf("Usage: %s <port> <max number of thread>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_nothread = atoi(argv[2]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener == -1){
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))){
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)){
        perror("listen() failed");
        return 1;
    }

    printf("Server started. Waiting for connections...\n");


    pthread_t thread_id;
    void *ret;
    for (int i=0;i<max_nothread;i++){
        printf("create thread %d\n", i);
        if (pthread_create(&thread_id, NULL, client_thread, (void *) &listener) != 0){
            perror("pthread_create() failed");
            sched_yield();
        }
    }
    if (pthread_join(thread_id, &ret) != 0){
        perror("pthread_create() failed");
        exit(3);
    }
    printf("Thread exited with  %s\n", ret);

    getchar();
    close(listener);
    return 0;
}

void* client_thread(void *arg){
    char *ret;
    int listener = *(int *)arg;

    char buf[1024];

    while (1){
        printf("New iter\n");
        int client = accept(listener, NULL, NULL);
        if (client==-1){
            perror("accept() failed");
            continue;
        } else {
            printf("New client %d accepted in thread %ld with pid %d\n", client, pthread_self(), getpid());

            int ret = recv(client, buf, sizeof(buf), 0);

            if (ret <= -1){
                perror("recv() failed");
                close(client);
                continue;
            } else if (ret == 0){
                printf("Client disconnected\n");
                close(client);
                continue;
            } else {
                buf[ret-1] = 0;
                puts(buf);
                char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
                send(client, msg, strlen(msg), 0);
                close(client);
                continue;
            }
        }
    }
}