#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 256

void handle_client(int client_sock, int data_fd)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    ssize_t bytes_written;

    // Send greeting message to client
    int greeting_fd = open("greeting.txt", O_RDONLY);
    if (greeting_fd == -1)
    {
        perror("open() failed");
        return;
    }

    while ((bytes_received = read(greeting_fd, buffer, BUFFER_SIZE)) > 0)
    {
        bytes_written = send(client_sock, buffer, bytes_received, 0);
        if (bytes_written == -1)
        {
            perror("send() failed");
            close(greeting_fd);
            return;
        }
    }

    close(greeting_fd);

    // Receive data from client and write to file
    while ((bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0)
    {
        bytes_written = write(data_fd, buffer, bytes_received);
        if (bytes_written == -1)
        {
            perror("write() failed");
            return;
        }
    }

    if (bytes_received == -1)
    {
        perror("recv() failed");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Usage: %s <port> <greeting_file> <data_file>\n", argv[0]);
        return 1;
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind() failed");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, MAX_CLIENTS) == -1)
    {
        perror("listen() failed");
        close(server_sock);
        return 1;
    }

    int data_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (data_fd == -1)
    {
        perror("open() failed");
        close(server_sock);
        return 1;
    }

    printf("Server started. Listening on port %s\n", argv[1]);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1)
        {
            perror("accept() failed");
            close(server_sock);
            close(data_fd);
            return 1;
        }

        char client_ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN) == NULL)
        {
            perror("inet_ntop() failed");
            close(client_sock);
            continue;
        }

        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        // Handle client communication in a new process
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork() failed");
            close(client_sock);
            continue;
        }

        if (pid == 0) // Child process
        {
            close(server_sock); // Close the listening socket in the child process
            handle_client(client_sock, data_fd);
            close(client_sock);
            exit(0);
        }

        // Parent process
        close(client_sock); // Close the client socket in the parent process
    }

    close(data_fd);
    close(server_sock);
return 0;
}
