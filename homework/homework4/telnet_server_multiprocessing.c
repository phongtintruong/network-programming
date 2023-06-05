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

int authenticate(char* username, char* password) {
    // Open the database file
    FILE* db = fopen("database.txt", "r");
    if (db == NULL) {
        perror("Failed to open database file");
        return 0;
    }

    // Read each line from the database file and compare with the client's credentials
    char line[256];
    while (fgets(line, sizeof(line), db) != NULL) {
        line[strcspn(line, "\n")] = '\0';  // Remove trailing newline character

        char* stored_username = strtok(line, " ");
        char* stored_password = strtok(NULL, " ");

        // printf("|%s| |%s| ; |%s| |%s|\n", stored_username, username, stored_password, password);

        if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
            fclose(db);
            return 1;  // Authentication successful
        }
    }

    fclose(db);
    return 0;  // Authentication failed
}

void get_time_to_buf(char* buf){
    // Get the current time
    time_t current_time;
    time(&current_time);

    // Format the time as "YYYY/MM/DD HH:MM:SSAM/PM"
    struct tm* time_info;
    time_info = localtime(&current_time);
    char time[32];
    strftime(time, 24, "[%Y/%m/%d %H:%M:%S%p]", time_info);
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
                get_time_to_buf(msg);
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
                    char username[100]="";
                    char password[100]="";
                    int is_login=0;
                    int login_failed_count=0;

                    get_time_to_buf(msg);
                    strcat(msg, " Enter username: ");
                    send(client, msg, strlen(msg), 0);
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
                                get_time_to_buf(msg);
                                strcat(msg, " Bye Bye.\n");
                                send(client, msg, strlen(msg), 0);

                                close(client);
                                exit(0);
                            } else {
                                if (strlen(username)==0){

                                    char get_username[32], tmp[32];
                                    ret = sscanf(buf, "%s%s", get_username, tmp);
                                    if (ret==1){
                                        strcpy(username, get_username);
                                        strcpy(msg, "");
                                        get_time_to_buf(msg);
                                        strcat(msg, " Enter password: ");
                                        send(client, msg, strlen(msg), 0);
                                    } else {
                                        strcpy(msg, "");
                                        get_time_to_buf(msg);
                                        strcat(msg, " Wrong usage. Username must not have space.\n");
                                        get_time_to_buf(msg);
                                        strcat(msg, " Enter username: ");
                                        send(client, msg, strlen(msg), 0);
                                    }
                                } else if (strlen(password)==0){
                                    char get_password[32], tmp[32];
                                    ret = sscanf(buf, "%s%s", get_password, tmp);
                                    if (ret==1){
                                        strcpy(password, get_password);

                                        if (authenticate(username, password)){
                                            is_login=1;
                                            strcpy(msg, "");
                                            get_time_to_buf(msg);
                                            strcat(msg, " Login successful.\n");
                                            send(client, msg, strlen(msg), 0);
                                        } else {
                                            if (login_failed_count<3){
                                                login_failed_count++;
                                                is_login=0;
                                                printf("client %d fail login %d time.\n", client, login_failed_count);
                                                strcpy(password, "");
                                                strcpy(username, "");
                                                strcpy(msg, "");
                                                get_time_to_buf(msg);
                                                strcat(msg, " Login failed.\n");
                                                get_time_to_buf(msg);
                                                strcat(msg, " Enter username: ");
                                                send(client, msg, strlen(msg), 0);
                                            } else {
                                                printf("client %d fail login %d time. Close client %d.\n", client, login_failed_count, client);
                                                strcpy(msg, "");
                                                get_time_to_buf(msg);
                                                strcat(msg, " Login failed 3 time. Comeback later.\n");
                                                send(client, msg, strlen(msg), 0);
                                                close(client);
                                                exit(0);
                                            }
                                        }
                                    } else {
                                        strcpy(msg, "");
                                        get_time_to_buf(msg);
                                        strcat(msg, " Wrong usage. Password must not have space.\n");
                                        get_time_to_buf(msg);
                                        strcat(msg, " Enter password: ");
                                        send(client, msg, strlen(msg), 0);
                                    }
                                } else if (is_login){
                                    char cmd[256];
                                    strcpy(cmd, buf);
                                    strcat(cmd, " > out.txt");
                                    system(cmd);
                                    FILE *f = fopen("out.txt", "r");
                                    int ret = 0;
                                    char sendbuf[1024];
                                    while (!feof(f)){
                                        ret = fread(sendbuf, 1, sizeof(sendbuf), f);
                                        if (ret<=0){
                                            break;
                                        }
                                        send(client, sendbuf, strlen(sendbuf), 0);
                                    }
                                    fclose(f);
                                    close(client);
                                    perror("something: ");
                                    printf("close client in child\n");
                                    exit(0);
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
