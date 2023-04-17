#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_DRIVES 10
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

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
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect() failed");
        close(client_sock);
        return 1;
    }

    // Get computer name from user
    char computer_name[BUFFER_SIZE];
    printf("Tên máy tính: ");
    fgets(computer_name, BUFFER_SIZE, stdin);
    computer_name[strlen(computer_name)-1] = '\0';
    // printf("%s", computer_name);
    // Get number of drive from user
    int num_drives;
    printf("Số ổ đĩa: ");
    scanf("%d", &num_drives);
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }



    // Get drive information from user
    char drive_info[MAX_DRIVES][BUFFER_SIZE];
    int index_drive = 0;

    while (index_drive < num_drives)
    {
        printf("Nhập thông tin drive %d: ", index_drive+1);
        fgets(drive_info[index_drive], BUFFER_SIZE, stdin);

        if (strcmp(drive_info[index_drive], "\n") == 0)  // Empty string signals end of input
        {
            break;
        }

        index_drive++;
    }

    // Send data to server
    char message[BUFFER_SIZE];
    int message_len = sprintf(message, "%s%d", computer_name, num_drives);

    for (int i = 0; i < num_drives; i++)
    {
        message_len += sprintf(message + message_len, " %s", drive_info[i]);
    }

    if (send(client_sock, message, message_len, 0) == -1)
    {
        perror("send() failed");
        close(client_sock);
        return 1;
    }

    close(client_sock);

    return 0;
}
