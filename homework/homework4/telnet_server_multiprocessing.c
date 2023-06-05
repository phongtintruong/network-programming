#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

int client_count=0;

void get_time_to_buf(char* buf){
    // Get the current time
    time_t current_time;
    time(&current_time);

    // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
    struct tm* time_info;
    time_info = localtime(&current_time);
    strftime(buf, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
}

void signalHandler(int signo){
    int pid = wait(NULL);
    printf("Child %d terminated.\n", pid);
    client_count--;
}

int main(int argc, char *argv[]){
    if (argc != 3){
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

    signal(SIGCHLD, signalHandler);


    while (1){
        printf("Waiting for new client...\n");
        int client = accept(listener, NULL, NULL);
        if (client==-1){
            perror("accept() failed");
            return 1;
        } else {
            if (client_count==max_noclient){
                char msg[256];
                get_time_to_buf(msg);
                strcat(msg, " number of clients limit exceeds, comeback later.\n");
                send(client, msg, strlen(msg), 0);
                printf("number of clients limit exceeds, excess client will not be processed\n");
                close(client);
            } else {
                printf("New user connected: %d\n", client);
                client_count++;
                    
                if (fork()==0){

                    close(listener);

                    char msg[256], buf[256];
                    get_time_to_buf(msg);
                    strcat(msg, " Enter username: ");
                    send(client, msg, strlen(msg), 0);
                    while (1){
                        int ret = recv(client, buf, sizeof(buf), 0);
                        if (ret <= -1){
                            perror("recv() failed");
                            close(client);
                            exit(1);
                        }
                    }
                }
            }
        }
       


        
    }
}
