#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>


int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <server_ip> <server_port> <buff_size> <input_file>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);
    int buf_size = atoi(argv[3]);

    // Create socket
    int client_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_sock == -1)
    {
        perror("socket() failed");
        return 1;
    }

    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed");
        close(client_sock);
        return 1;
    }


    // read input file 
    char buf[buf_size];

    FILE *fp;

    fp = fopen(argv[4], "r");
    if (fp == NULL){
        perror("Can't open file");
        return 1;
    }
    
    while (!feof(fp)){
        int bytes_read = fread(buf, 1, buf_size, fp);
        if (bytes_read<=0){
            break;
        }
        // buf[bytes_read]='\n';
        if (send(client_sock, buf, bytes_read, 0)<0){
            perror("send() failed");
            return 1;
        }
    }

    fclose(fp);
    close(client_sock);

    return 0;
}