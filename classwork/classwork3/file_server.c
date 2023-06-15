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
// #define _DEFAULT_SOURCE

#include <dirent.h>

int client_count=0;

// void get_time_to_buf(char* buf, char* format){
//     // Get the current time
//     time_t current_time;
//     time(&current_time);

//     // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
//     struct tm* time_info;
//     time_info = localtime(&current_time);
//     char time[32];
//     if (strcmp(format, "default")==0){
//         strftime(time, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
//     } else {
//         switch (is_format(format))
//         {
//         case 0:
//             strftime(time, 24, "%d/%m/%Y", time_info);
//             break;
//         case 1:
//             strftime(time, 24, "%d/%m/%y", time_info);
//             break;
//         case 2:
//             strftime(time, 24, "%m/%d/%Y", time_info);
//             break;
//         case 3:
//             strftime(time, 24, "%m/%d/%y", time_info);
//             break;
//         }
//     }
//     strcat(buf, time);
// }

void signalHandler(int signo){
    int pid = wait(NULL);
    printf("Child %d terminated.\n", pid);
    client_count--;
}

int main(int argc, char *argv[]){
    if (argc != 4){
        printf("Usage: %s <port> <max number of client> <dir>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    int max_noclient = atoi(argv[2]);
    char search_dir[32];
    strcpy(search_dir, argv[3]);

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
    
    char *file_list = malloc(1);
    file_list[0] = '\0';

    DIR *dir = opendir(search_dir);
    if (dir==NULL){
        perror("Failed to open dir");
        return 1;
    }

    struct dirent *entry;
    int count=0;
    char count_str[20];
    while ((entry = readdir(dir))!=NULL){
        if (entry->d_type==DT_REG){
            count++;
            file_list = realloc(file_list, strlen(file_list)+strlen(entry->d_name)+3);
            strcat(file_list, entry->d_name);
            strcat(file_list, "\r\n");
        }
    }
    closedir(dir);

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
                // get_time_to_buf(msg, "default");
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

                    if (count>0){
                        sprintf(count_str, "%d", count);
                        strcpy(msg, "");
                        strcat(file_list, "\r\n");
                        strcat(msg, "OK ");
                        strcat(msg, count_str);
                        strcat(msg, "\r\n");
                        strcat(msg, file_list);
                        send(client, msg, strlen(msg), 0);
                    } else {
                        printf("nofile\n");
                        strcat(msg, "ERROR No files to download\r\n");
                        send(client, msg, strlen(msg), 0);
                        close(client);
                        exit(0);
                    }

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
                                // get_time_to_buf(msg, "default");
                                strcat(msg, " Bye Bye.\n");
                                send(client, msg, strlen(msg), 0);

                                close(client);
                                exit(0);
                            } else {
                                char file_name[32], tmp[32];
                                ret = sscanf(buf, "%s%s", file_name, tmp);
                                if (ret==1){
                                    FILE *f = fopen(buf, "rb");
                                    if (f==NULL){
                                        strcpy(msg, "");
                                        strcat(msg, "ERROR Can't open file.\n");
                                        send(client, msg, strlen(msg), 0);
                                    } else {
                                        fseek(f, 0, SEEK_END);
                                        long file_size = ftell(f);
                                        fseek(f, 0, SEEK_SET);

                                        sprintf(buf, "OK %ld\r\n", file_size);
                                        send(client, buf, strlen(buf), 0);
                                        
                                        while (1){
                                            ret = fread(buf, 1, sizeof(buf), f);
                                            if (ret<=0){
                                                printf("End of file.\n");
                                                break;
                                            }
                                            send(client, buf, ret, 0);
                                        }
                                        
                                        fclose(f);
                                        break;
                                    }


                                } else {
                                    strcpy(msg, "");
                                    // get_time_to_buf(msg, "default");
                                    strcat(msg, "Wrong usage.\n");
                                    
                                    send(client, msg, strlen(msg), 0);
                                }
                            }
                        }
                    }
                    close(client);
                    exit(0);
                }
                close(client);
            }
        }
    }
    close(listener);
    return 0;
}
