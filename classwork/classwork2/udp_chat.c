#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <sys/select.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s  <port>\n", argv[0]);
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

    fd_set fdread;
    int clients[max_noclient];
    char *client_id[max_noclient];
    int client_state[max_noclient];
    for (int i=0;i<max_noclient;i++){
        client_state[i]=1;
    }
    int num_clients=0;
    char buf[256];
    struct timeval tv;

    while(1){
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);

        int maxdp = listener;
        for (int i=0;i<num_clients;i++){
            FD_SET(clients[i],&fdread);
            if (clients[i]>maxdp){
                maxdp=clients[i];
            }
        }

        tv.tv_sec=60;
        tv.tv_usec=0;

        int ret = select(maxdp+1, &fdread, NULL, NULL, &tv);
        if (ret<0){
            perror("select() failed");
            return 1;
        }

        if (ret==0){
            printf("TIME OUT!!!\n");
            continue;
        }

        if (FD_ISSET(listener, &fdread)){
            int client = accept(listener, NULL, NULL);
            if (client==-1){
                perror("accept() failed");
                // continue;
            } else {
                if (num_clients==max_noclient){
                    int check = 0;
                    for (int i=0;i<max_noclient;i++){
                        if (client_state[i]==0){
                            check++;
                            printf("New user connected: %d\n", client);
                            clients[i] = client;
                            client_state[i]=1;
                            break;                            
                        }
                    }
                    if (check==0){
                        char *msg = "number of clients limit exceeds, comeback later.\n";
                        send(client, msg, strlen(msg), 0);
                        printf("number of clients limit exceeds, excess client will not be processed\n");
                        close(client);
                    }
                } else {
                    printf("New user connected: %d\n", client);
                    clients[num_clients]=client;
                    client_state[num_clients]=1;
                    num_clients++;
                }
            }
        }

        for (int i=0;i<num_clients;i++){
            if (FD_ISSET(clients[i], &fdread)){
                ret = recv(clients[i], buf, sizeof(buf), 0);
                if (ret <= -1){
                    perror("recv() failed");
                    FD_CLR(clients[i], &fdread);
                    close(clients[i]);
                    client_state[i]=0;
                    clients[i]=-1;
                    continue;
                } else if (ret==0){
                    perror("client disconnected");
                    FD_CLR(clients[i], &fdread);
                    close(clients[i]);
                    client_state[i]=0;
                    clients[i]=-1;
                    continue;
                } else {
                    buf[ret] = 0;
                    printf("Received from %d: %s\n", clients[i], buf);
                    if (client_state[i]==1){
                        char cmd[32], id[32], tmp[32];
                        ret = sscanf(buf, "%s%s%s", cmd, id, tmp);
                        if (ret==2){
                            if (strcmp(cmd, "client_id:")==0){
                                char *msg = "You are signed in. Enter message.\n";
                                send(clients[i], msg, strlen(msg), 0);

                                client_id[i]=malloc(strlen(id)+1);
                                strcpy(client_id[i], id);
                                client_state[i]=2;
                            } else {
                                char *msg = "Wrong usage. Usage: client_id: <id>.\n";
                                send(clients[i], msg, strlen(msg), 0);
                            }
                        } else {
                            char *msg = "Wrong usage. Usage: client_id: <id>.\n";
                            send(clients[i], msg, strlen(msg), 0);
                        }
                    } else {
                        char sendbuf[256];

                        strcpy(sendbuf, client_id[i]);
                        strcat(sendbuf, ": ");
                        strcat(sendbuf, buf);

                        for (int j=0;j<num_clients;j++){
                            if (client_state[j]==2&&j!=i){
                                send(clients[j], sendbuf, strlen(sendbuf), 0);
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