#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return 1;
    }

    char *ip = argv[1];
    int port = atoi(argv[2]);

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

    char buf[256];

    // Get computer name from user
    char computer_name[64];
    printf("Enter computer name: ");
    scanf("%s",computer_name);
    getchar();

    // fgets(computer_name, 64, stdin);
    // computer_name[strlen(computer_name)-1] = '\0';
    // printf("%s", computer_name);

    // Put in buff
    strcpy(buf, computer_name);
    int pos = strlen(computer_name);
    buf[pos]=0;
    pos++;

    // Get number of drive from user
    int num_disks;
    printf("Enter number of disk: ");
    scanf("%d", &num_disks);
    getchar();
    // int c;
    // while ((c = getchar()) != '\n' && c != EOF) { }

    // Get drive information from user
    char disk_letter;
    short int disk_size;

    for (int i=0; i<num_disks;i++){
        printf("Enter disk name: ");
        scanf("%c",&disk_letter);

        printf("Enter storage of disk: ");
        scanf("%hd", &disk_size);

        getchar();

        // Put in buff
        buf[pos]=disk_letter;
        pos++;
        memcpy(buf + pos, &disk_size, sizeof(disk_size));
        pos+=sizeof(disk_size);
    }
    
    // Send data to server
    printf("Buffer size: %d\n",pos);
    if (send(client_sock, buf, pos, 0) == -1)
    {
        perror("send() failed");
        close(client_sock);
        return 1;
    }
    // char message[BUFFER_SIZE];
    // int message_len = sprintf(message, "%s%d", computer_name, num_drives);

    // for (int i = 0; i < num_drives; i++)
    // {
    //     message_len += sprintf(message + message_len, " %s", drive_info[i]);
    // }


    close(client_sock);

    return 0;
}
