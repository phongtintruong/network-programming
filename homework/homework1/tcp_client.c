#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <IP address> <port>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(port);

    int ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect() failed");
        close(client);
        return 1;
    }

    char buf[256];
    // int bytes_received;

    while (1)
    {
        printf("Enter string:");
        fgets(buf, sizeof(buf), stdin);

        send(client, buf, strlen(buf), 0);

        if (strncmp(buf, "exit", 4) == 0)
            break;

        // bytes_received = recv(client, buf, sizeof(buf), 0);
        // if (bytes_received <= 0)
        // {
        //     printf("Connection closed\n");
        //     break;
        // }

        // if (bytes_received < sizeof(buf))
        //     buf[bytes_received] = '\0';

        // printf("%d bytes received: %s\n", bytes_received, buf);
    }

    close(client);
    return 0;
}
