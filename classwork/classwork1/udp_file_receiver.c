#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <string.h>

struct client_info{
    char clientAddrStr[24];
    char filename[256];
    long filesize;
    FILE *file;
};

struct client_info clients[32];
int num_clients = 0;

int main(int argc, char *argv[]){
    if (argc!=2){
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int receiver = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (receiver == -1){
        perror("Socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(receiver, (struct sockaddr *)&addr, sizeof(addr))){
        perror("bind() failed");
        return 1;
    }

    int buf_size = 2000;
    char buf[buf_size];
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char clientAddrStr[24];
    while (1){
        int ret = recvfrom(receiver, buf, sizeof(buf), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (ret<0){
            perror("recvfrom() failed");
            return 1;
        }

        sprintf(clientAddrStr, "%s:%d", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

        int i=0;
        for (;i<num_clients;i++){
            if (strcmp(clients[i].clientAddrStr, clientAddrStr)==0){
                break;
            }
        }

        if (i==num_clients){
            struct client_info new_client;
            strcpy(new_client.clientAddrStr, clientAddrStr);
            memcpy(&new_client.filesize, buf, sizeof(new_client.filesize));
            strcpy(new_client.filename, "recieved_");
            strcat(new_client.filename, buf+sizeof(new_client.filesize));
            new_client.file = fopen(new_client.filename, "wb");
            clients[i]=new_client;
            num_clients++;
            printf("Receive %s from %s\n", new_client.filename, new_client.clientAddrStr);
        }
        else{
            fwrite(buf, 1, ret, clients[i].file);
            if (ftell(clients[i].file)==clients[i].filesize){
                fclose(clients[i].file);

                printf("Finish receiving %s from %s\n",clients[i].filename, clients[i].clientAddrStr);

                if (i<num_clients-1){
                    clients[i] = clients[num_clients-1];
                }
                num_clients--;
            }

        }

    }
    close(receiver);
    return 0;
}