#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <poll.h>
#include <time.h>
#include <ctype.h>

void get_time_to_buf(char* buf){
    // Get the current time
    time_t current_time;
    time(&current_time);

    // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
    struct tm* time_info;
    time_info = localtime(&current_time);
    strftime(buf, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
}

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

    struct pollfd fds[max_noclient+1];
    
    int nfds = 1;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    char buf[256];

    while(1){
        
        int ret = poll(fds, nfds, 60000);
        if (ret < 0){
            perror("poll() failed");
            break;
        }

        if (ret == 0){
            printf("TIME OUT!!!\n");
            continue;
        }

        if (fds[0].revents & POLLIN){
            int client = accept(listener, NULL, NULL);
            if (client==-1){
                perror("accept() failed");
                // continue;
            } else {
                if (nfds>max_noclient){
                    int check = 0;
                    // for (int i=0;i<max_noclient;i++){
                    //     if (client_state[i]==0){
                    //         check++;
                    //         printf("New user connected: %d\n", client);
                    //         clients[i] = client;
                    //         client_state[i]=1;
                    //         break;                            
                    //     }
                    // }
                    if (check==0){
                        char msg[256];
                        get_time_to_buf(msg);
                        strcat(msg, " number of clients limit exceeds, comeback later.\n");
                        send(client, msg, strlen(msg), 0);
                        printf("number of clients limit exceeds, excess client will not be processed\n");
                        close(client);
                    }
                } else {
                    printf("New user connected: %d\n", client);
                    char msg[256];
                    get_time_to_buf(msg);
                    strcat(msg, " Welcome to server mfk. There are ");
                    char nfds_string[2];
                    sprintf(nfds_string, "%d", nfds-1);
                    strcat(msg, nfds_string);
                    strcat(msg, " mfks here.\n");
                    send(client, msg, strlen(msg), 0);
                    fds[nfds].fd=client;
                    fds[nfds].events = POLLIN;
                    nfds++;
                }
            }
        }


        for (int i=1;i<nfds;i++){
            if (fds[i].revents & POLLIN){
                ret = recv(fds[i].fd, buf, sizeof(buf), 0);
                if (ret <= -1){
                    perror("recv() failed");
                    close(fds[i].fd);
                    if (i<nfds-1){
                        fds[i] = fds[nfds-1];
                    }
                    nfds--;
                    i--;
                } else if (ret==0){
                    perror("client disconnected");
                    close(fds[i].fd);
                    if (i<nfds-1){
                        fds[i] = fds[nfds-1];
                    }
                    nfds--;
                    i--;
                } else {
                    buf[ret] = 0;
                    printf("Received from %d: %s\n", fds[i].fd, buf);
                    
                    char sendbuf[1024];
                    char *token = strtok(buf, " ");
                    while (token != NULL){
                        token[0]=toupper(token[0]);
                        for (int i=1;i<strlen(token);i++){
                            token[i]=tolower(token[i]);
                        }
                        strcat(sendbuf, token);
                        strcat(sendbuf, " ");
                        token=strtok(NULL, " ");
                    }
                    sendbuf[strlen(sendbuf)-1]=0;
                    send(fds[i].fd, sendbuf, strlen(sendbuf), 0);
                }
            }
        }

    }
    close(listener);
    return 0;
}