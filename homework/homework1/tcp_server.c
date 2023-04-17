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
    if (argc!=4){
        printf("Usage: %s <port> <greeting file> <output file>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

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
    int clientAddrlen = sizeof(addr);

    int client = accept(server, (struct sockaddr *)&clientAddr, &clientAddrlen);

    if (client==-1){
        perror("Accept() failed");
        return 1;
    }

    printf("Client IP: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    int greeting = open(argv[2], O_RDONLY);

    if (greeting==-1){
        perror("Open() failed");
        return 1;
    }

    char greeting_msg[256];
    int bytes_read = read(greeting, greeting_msg, sizeof(greeting_msg));
    if (bytes_read == -1)
    {
        perror("read() failed");
        return 1;
    }
    greeting_msg[bytes_read] = '\n';

    int ret = send(client, greeting_msg, strlen(greeting_msg), 0);
    if (ret != -1){
        printf("%d bytes are sent\n", bytes_read);
    }

    int output = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output == -1){
        perror("open() failed");
        return 1;
    }

    char buf[256];
    bytes_read = recv(client, buf, sizeof(buf), 0);
    while (bytes_read > 0)
    {
        buf[bytes_read] = '\0';
        printf("Received %d bytes: %s\n", bytes_read, buf);

        ret = write(output, buf, bytes_read);
        if (ret == -1)
        {
            perror("write() failed");
            return 1;
        }

        bytes_read = recv(client, buf, sizeof(buf), 0);
    }

    if (bytes_read == -1)
    {
        perror("recv() failed");
        return 1;
    }

    printf("Connection closed\n");

    close(output);
    close(client);
    close(server);

    return 0;

}