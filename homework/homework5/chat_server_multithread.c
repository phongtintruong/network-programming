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

int client_count=0;
pthread_mutex_t client_count_mutex = PTHREAD_MUTEX_INITIALIZER;

void get_time_to_buf(char* buf){
    // Get the current time
    time_t current_time;
    time(&current_time);

    // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
    struct tm* time_info;
    time_info = localtime(&current_time);
    strftime(buf, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
}

void *client_thread(void *arg);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <max number of client>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_noclient = atoi(argv[2]);

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


    while (1){
        int client = accept(listener, NULL, NULL);
        if (client==-1){
            perror("accept() failed");
        } else {
            if (client_count>=max_noclient){
                char msg[256];
                get_time_to_buf(msg);
                strcat(msg, " number of clients limit exceeds, comeback later.\n");
                send(client, msg, strlen(msg), 0);
                printf("number of clients limit exceeds, excess client will not be processed\n");
                close(client);
            } else {
                void *ret;
                printf("New user connected: %d\n", client);
                pthread_t thread_id;
                client_count++;
                if(pthread_create(&thread_id, NULL, client_thread, (void *)&client)!=0){
                    perror("pthread_create() failed");
                    return 1;
                }
                if (pthread_join(thread_id, &ret)!=0){
                    perror("pthread_create() failed");
                    return 1;
                }
                pthread_detach(thread_id);
                printf("Thread exited with '%s'\n", (char *)ret);
            }
        }
    }
    close(listener);
    return 0;    
}

void *client_thread(void *param){
    char *client_thread_ret;
    int client = *(int *)param;
    char buf[256];

    while (1){
        int ret = recv(client, buf, sizeof(buf), 0);
        if (ret<=-1){
            perror("recv() failed");
            close(client);
            strcpy(client_thread_ret, "recv() failed");
            pthread_exit(client_thread_ret);
        }
    }
}
