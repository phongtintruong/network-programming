#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>


int main(int argc, char *argv[]){
    if (argc != 3){
        printf("Usage: %s <port> <max number of process>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_noprocess = atoi(argv[2]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listener == -1){
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))){
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)){
        perror("listen() failed");
        return 1;
    }

    printf("Server started. Waiting for connections...\n");
    for (int i=0;i<max_noprocess;i++){
        if (fork()==0){
            char buf[256];
            while (1){
                int client = accept(listener, NULL, NULL);
                printf("New client connected: %d\n", client);

                int ret = recv(client, buf, sizeof(buf), 0);

                if (ret<=0){
                    close(client);
                    continue;
                }

                buf[ret]=0;
                puts(buf);
                char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
                send(client, msg, strlen(msg), 0);

                close(client);
            }
        }
    }
    getchar();
    close(listener);
    killpg(0, SIGKILL);

    return 0;
}