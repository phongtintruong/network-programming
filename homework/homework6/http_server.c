#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define PORT 9000
#define BUFFER_SIZE 1024

void send_response(int client_socket, const char *response) {
    send(client_socket, response, strlen(response), 0);
}

void send_file_content(int client_socket, const char *file_path) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        send_response(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

void handle_request(int client_socket) {
    char request[BUFFER_SIZE];
    recv(client_socket, request, sizeof(request), 0);

    // Parse the request to get the requested file path
    char *method = strtok(request, " ");
    char *path = strtok(NULL, " ");

    if (strcmp(method, "GET") == 0) {
        char file_path[256];
        sprintf(file_path, ".%s", path);

        struct stat file_info;
        if (stat(file_path, &file_info) == 0) {
            if (S_ISDIR(file_info.st_mode)) {
                // Serve the directory content
                DIR *dir = opendir(file_path);
                struct dirent *dir_entry;
                char response[BUFFER_SIZE];

                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
                send_response(client_socket, response);

                while ((dir_entry = readdir(dir)) != NULL) {
                    if (dir_entry->d_type == DT_DIR) {
                        sprintf(response, "<a href=\"%s/\"><strong>%s/</strong></a><br>", dir_entry->d_name, dir_entry->d_name);
                        send_response(client_socket, response);
                    } else {
                        sprintf(response, "<a href=\"%s\"><em>%s</em></a><br>", dir_entry->d_name, dir_entry->d_name);
                        send_response(client_socket, response);
                    }
                }

                closedir(dir);
            } else {
                // Serve the file content
                send_response(client_socket, "HTTP/1.1 200 OK\r\n\r\n");
                send_file_content(client_socket, file_path);
            }
        } else {
            send_response(client_socket, "HTTP/1.1 404 Not Found\r\n\r\n");
        }
    } else {
        send_response(client_socket, "HTTP/1.1 400 Bad Request\r\n\r\n");
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length = sizeof(client_address);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error: socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error: socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_socket, 3) < 0) {
        perror("Error: socket listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started. Listening on port %d\n", PORT);

    while (1) {
        // Accept incoming connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("Error: failed to accept connection");
            exit(EXIT_FAILURE);
        }

        // Handle the request in a separate process
        if (fork() == 0) {
            handle_request(client_socket);
            exit(EXIT_SUCCESS);
        } else {
            close(client_socket);
        }
    }

    close(server_socket);

    return 0;
}
