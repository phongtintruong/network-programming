#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

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
        
    // Truyen nhan du lieu
    char mssv[9];
    char hoten[64];
    char ns[11];
    float dtb;
    char buf[256];

    while (1)
    {
        printf("Nhap thong tin cua sinh vien:\n");
        printf("Nhap MSSV: ");
        scanf("%s", mssv);
        
        // Xoa \n trong bo dem
        getchar();

        if (strncmp(mssv, "0000", 4) == 0)
            break;

        printf("Nhap Ho ten: ");
        fgets(hoten, sizeof(hoten), stdin);
        hoten[strlen(hoten) - 1] = 0;

        printf("Nhap Ngay sinh: ");
        scanf("%s", ns);

        // Xoa \n trong bo dem
        getchar();

        printf("Nhap Diem TB: ");
        scanf("%f", &dtb);

        // Xoa \n trong bo dem
        getchar();

        sprintf(buf, "%s %s %s %.2f", mssv, hoten, ns, dtb);
        send(client, buf, strlen(buf), 0);
    }

    // Ket thuc, dong socket
    close(client);

    return 0;
}