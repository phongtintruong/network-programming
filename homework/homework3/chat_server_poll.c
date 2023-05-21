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
    char *client_id[max_noclient+1];
    int client_state[max_noclient+1];
    for (int i=1;i<max_noclient+1;i++){
        client_state[i]=0; //Haven't login yet
    }
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
                        free(client_id[i]);
                        fds[i] = fds[nfds-1];
                        client_id[i]=malloc(strlen(client_id[nfds-1])+1);
                        strcpy(client_id[i], client_id[nfds-1]);
                    }
                    free(client_id[nfds-1]);
                    client_state[nfds-1]=0;
                    nfds--;
                    i--;
                } else if (ret==0){
                    perror("client disconnected");
                    close(fds[i].fd);
                    if (i<nfds-1){
                        free(client_id[i]);
                        fds[i] = fds[nfds-1];
                        client_id[i]=malloc(strlen(client_id[nfds-1])+1);
                        strcpy(client_id[i], client_id[nfds-1]);
                    }
                    free(client_id[nfds-1]);
                    client_state[nfds-1]=0;
                    nfds--;
                    i--;
                } else {
                    buf[ret] = 0;
                    printf("Received from %d: %s\n", fds[i].fd, buf);
                    
                    if (client_state[i]==0){
                        char cmd[32], id[32], tmp[32];
                        ret = sscanf(buf, "%s%s%s", cmd, id, tmp);
                        if (ret==2){
                            if (strcmp(cmd, "client_id:")==0){
                                char msg[256];
                                get_time_to_buf(msg);
                                strcat(msg, " You are signed in. Enter message.\n");
                                send(fds[i].fd, msg, strlen(msg), 0);

                                client_id[i]=malloc(strlen(id)+1);
                                strcpy(client_id[i], id);
                                client_state[i]=1;
                            } else {
                                char msg[256];
                                get_time_to_buf(msg);
                                strcat(msg, " Wrong usage. Usage: client_id: <id>.\n");
                                send(fds[i].fd, msg, strlen(msg), 0);
                            }
                        } else {
                            char msg[256];
                            get_time_to_buf(msg);
                            strcat(msg, " Wrong usage. Usage: client_id: <id>.\n");
                            send(fds[i].fd, msg, strlen(msg), 0);
                        }
                    } else {
                        char sendbuf[256];

                        get_time_to_buf(sendbuf);
                        strcat(sendbuf, " ");
                        strcat(sendbuf, client_id[i]);
                        strcat(sendbuf, ": ");
                        strcat(sendbuf, buf);

                        for (int j=1;j<nfds;j++){
                            if (client_state[j]==1&&j!=i){
                                send(fds[j].fd, sendbuf, strlen(sendbuf), 0);
                            }
                        }
                    }
                }
            }
        }

    }
    close(listener);
    return 0;
}