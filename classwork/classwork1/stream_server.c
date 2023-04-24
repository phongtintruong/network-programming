#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
    if (argc!=3){
        printf("Usage: %s <port> <buff_size>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int buf_size = atoi(argv[2]);

    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server == -1){
        perror("Socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(server, (struct sockaddr *)&addr, sizeof(addr))){
        perror("bind() failed");
        return 1;
    }

    if (listen(server, 5)){ // max_clients is 5
        perror("listen() failed");
        return 1;
    }

    struct sockaddr_in clientAddr;
    int clientAddrlen = sizeof(clientAddr);

    int client = accept(server, (struct sockaddr *)&clientAddr, &clientAddrlen);

    if (client==-1){
        perror("Accept() failed");
        return 1;
    }

    printf("Client IP: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    char *des_str="0123456789";
    int padding_len = strlen(des_str)-1;
    char buf[buf_size];
    memset(buf, 32, padding_len);
    int count = 0;

    while(1){
        int ret = recv(client, buf+padding_len, buf_size, 0);
        if (ret<=0){
            break;
        }

        char *p = strstr(buf, des_str);
        while (p != NULL && (p-buf)<ret){
            count++;
            p = strstr(p+1, des_str);
        }

        memcpy(buf, buf+ret, padding_len);
    }

    printf("Number of substring: %d\n", count);

    printf("Connection closed\n");

    close(client);
    close(server);

    return 0;

}