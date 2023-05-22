#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFFER_SIZE 4096

void chat_client(const char* server_ip, int server_port, int client_port) {
    // Tạo socket cho máy nhận và máy gửi
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;

    // Tạo socket cho máy nhận
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(client_port);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding server socket");
        exit(1);
    }

    // Tạo socket cho máy gửi
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0) {
        perror("Error creating client socket");
        exit(1);
    }

    memset(&client_address, 0, sizeof(client_address));
    client_address.sin_family = AF_INET;
    client_address.sin_addr.s_addr = inet_addr(server_ip);
    client_address.sin_port = htons(server_port);

    // Tạo danh sách các file descriptor cần theo dõi
    fd_set read_fds;
    int max_fd = (server_socket > STDIN_FILENO) ? server_socket : STDIN_FILENO;
    char buffer[BUFFER_SIZE];

    printf("Chat client started on port %d\n", client_port);
    printf("Type 'exit' to quit.\n");

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server_socket, &read_fds);

        // Sử dụng select để theo dõi các socket có sẵn để đọc
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("Error in select");
            exit(1);
        }

        // Nhận dữ liệu từ server
        if (FD_ISSET(server_socket, &read_fds)) {
            struct sockaddr_in sender_address;
            socklen_t sender_address_len = sizeof(sender_address);
            int bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&sender_address, &sender_address_len);

            if (bytes_received < 0) {
                perror("Error receiving data from server");
                exit(1);
            }

            buffer[bytes_received] = '\0';
            printf("[%s][%d]: %s\n", server_ip, server_port, buffer);
        }

        // Gửi dữ liệu từ client
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            fgets(buffer, BUFFER_SIZE, stdin);

            // Xóa ký tự newline từ buffer
            int len = strlen(buffer);
            if (buffer[len - 1] == '\n') {
                buffer[len - 1] = '\0';
            }

            sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&client_address, sizeof(client_address));

            // Kiểm tra điều kiện thoát
            if (strcmp(buffer, "exit") == 0) {
                close(client_socket);
                close(server_socket);
                exit(0);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <server_ip> <server_port> <client_port>\n", argv[0]);
        exit(1);
    }

    const char* server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int client_port = atoi(argv[3]);

    chat_client(server_ip, server_port, client_port);

    return 0;
}