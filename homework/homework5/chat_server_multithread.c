#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <time.h>
#include <pthread.h>

void get_time_to_buf(char* buf){
    // Get the current time
    time_t current_time;
    time(&current_time);

    // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
    struct tm* time_info;
    time_info = localtime(&current_time);
    strftime(buf, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <max number of client>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_noclient = atoi(argv[2]);

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

PTHREAD_MUTEX_INITIALIZER
