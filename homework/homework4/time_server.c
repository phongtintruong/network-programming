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

int client_count=0;

int is_format(char* format){
    if (strcmp(format, "dd/mm/yyyy")==0){
        return 0;
    }
    if (strcmp(format, "dd/mm/yy")==0){
        return 1;
    }
    if (strcmp(format, "mm/dd/yyyy")==0){
        return 2;
    }
    if (strcmp(format, "mm/dd/yy")==0){
        return 3;
    }
    return -1;
}

void get_time_to_buf(char* buf, char* format){
    // Get the current time
    time_t current_time;
    time(&current_time);

    // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
    struct tm* time_info;
    time_info = localtime(&current_time);
    char time[32];
    if (strcmp(format, "default")==0){
        strftime(time, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
    } else {
        switch (is_format(format))
        {
        case 0:
            strftime(time, 24, "%d/%m/%Y", time_info);
            break;
        case 1:
            strftime(time, 24, "%d/%m/%y", time_info);
            break;
        case 2:
            strftime(time, 24, "%m/%d/%Y", time_info);
            break;
        case 3:
            strftime(time, 24, "%m/%d/%y", time_info);
            break;
        }
    }
    strcat(buf, time);
}

void signalHandler(int signo){
    int pid = wait(NULL);
    printf("Child %d terminated.\n", pid);
    client_count--;
}

int main(int argc, char *argv[]){
    if (argc != 3){
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

    signal(SIGCHLD, signalHandler);
    

    while (1){
        
        printf("Waiting for new client...\n");
        int client = accept(listener, NULL, NULL);
        if (client==-1){
            perror("accept() failed");
            return 1;
        } else {
            if (client_count==max_noclient){
                char msg[256];
                get_time_to_buf(msg, "default");
                strcat(msg, " number of clients limit exceeds, comeback later.\n");
                send(client, msg, strlen(msg), 0);
                printf("number of clients limit exceeds, excess client will not be processed\n");
                close(client);
            } else {
                printf("New user connected: %d\n", client);
                client_count++;
                    
                if (fork()==0){
                    close(listener);

                    char msg[256]="", buf[256]="";

                    while (1){
                        int ret = recv(client, buf, sizeof(buf)-1, 0);
                        if (ret <= -1){
                            perror("recv() failed");
                            close(client);
                            exit(1);
                        } else if (ret==0){
                            printf("client disconnected\n");
                            close(client);
                            exit(0);
                        } else {
                            buf[ret-1] = 0;
                            printf("Received from %d: %s\n", client, buf);

                            if (strcmp(buf, "exit")==0){
                                strcpy(msg, "");
                                get_time_to_buf(msg, "default");
                                strcat(msg, " Bye Bye.\n");
                                send(client, msg, strlen(msg), 0);

                                close(client);
                                exit(0);
                            } else {
                                char cmd[32], format[32], tmp[32];
                                ret = sscanf(buf, "%s%s%s", cmd, format, tmp);
                                if (ret==2&&strcmp(cmd, "GET_TIME")==0){
                                    if (is_format(format)==-1){
                                        strcpy(msg, "");
                                        get_time_to_buf(msg, "default");
                                        strcat(msg, " Wrong format. Types of format:\n");
                                        strcat(msg, " - dd/mm/yyyy\n");
                                        strcat(msg, " - dd/mm/yy\n");
                                        strcat(msg, " - mm/dd/yyyy\n");
                                        strcat(msg, " - mm/dd/yy\n");
                                        send(client, msg, strlen(msg), 0);
                                    } else {
                                        strcpy(msg, "");
                                        get_time_to_buf(msg, format);
                                        send(client, msg, strlen(msg), 0);
                                        printf("done\n");
                                        close(client);
                                        exit(0);
                                    }
                                } else {
                                    strcpy(msg, "");
                                    get_time_to_buf(msg, "default");
                                    strcat(msg, " Wrong usage. Usage: GET_TIME [format].\n");
                                    
                                    send(client, msg, strlen(msg), 0);
                                }
                            }
                        }
                    }
                }
                close(client);
            }
        }
    }
    close(listener);
    return 0;
}
