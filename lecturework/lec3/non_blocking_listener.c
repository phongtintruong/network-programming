// example on page 147, 148
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/ioctl.h>

int queue[5]={0};
char buf[256];
int numClients = 0;
int clients[5];

int main(int argc, char *argv[]){
    if (argc!=2){
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener == -1){
        perror("Socket() failed");
        return 1;
    }

    int ul = 1;
    ioctl(listener, FIONBIO, &ul);

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


    // struct sockaddr_in clientAddr;
    // int clientAddrlen = sizeof(clientAddr);

    while(1){
        int client = accept(listener, NULL, NULL);
        if (client == -1){
            if (errno != EWOULDBLOCK && errno != EAGAIN){
                perror("accept() failed"); 
                return 1;
            } else {
                // Error by incomplete io. No need to handle 
            }
        } else {
            if (numClients==5){
                int check=0;
                for(int i=0;i<5;i++){
                    if (queue[i]==1){
                        check++;
                        printf("New client: %d\n", client);
                        clients[i] = client;
                        queue[i]=0;
                        ul = 1;
                        ioctl(client, FIONBIO, &ul);
                        break;
                    }
                }
                if (check==0){
                    perror("numClients limit exceeds, excess client will not be process");
                }
            } else {
                printf("New client: %d\n", client);
                clients[numClients++] = client;
                ul = 1;
                ioctl(client, FIONBIO, &ul);
            }
        }

        for (int i=0;i<numClients;i++){
            if (queue[i]==0){
                int ret = recv(clients[i], buf, sizeof(buf), 0);
                if (ret == -1){
                    if (errno != EWOULDBLOCK && errno != EAGAIN){
                        perror("recv() failed\n");
                        close(clients[i]);
                        queue[i]=1;
                        continue;
                    } else {
                        // Error by incomplete io. No need to handle 
                    }
                } else if (ret == 0){
                    perror("client disconnected\n");
                    queue[i]=1;
                    close(clients[i]);
                    continue;
                } else {
                    buf[ret] = 0;
                    printf("Received message from client %d: %s\n", clients[i], buf);
                }
            }
        }
            
    }
    close(listener);

    return 0;
}