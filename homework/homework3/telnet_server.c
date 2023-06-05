#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string.h>
#include <poll.h>
#include <time.h>

struct Client {
    int is_login;
    int login_failed_count;
    char username[100];
    char password[100];
};

int authenticate(struct Client* client) {
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

        if (strcmp(client->username, stored_username) == 0 && strcmp(client->password, stored_password) == 0) {
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

    struct pollfd fds[max_noclient+1];
    memset(fds, 0, sizeof(fds));
    struct Client clients[max_noclient];
    memset(clients, 0, sizeof(clients));
    int client_count = 0;

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    // char buf[256];

    while(1){
        
        int ret = poll(fds, client_count+1, 60000);
        if (ret < 0){
            perror("poll() failed");
            break;
        }

        if (ret == 0){
            printf("TIME OUT!!!\n");
            continue;
        }

        if (fds[0].revents & POLLIN){
            int client = accept(listener, NULL, NULL);
            if (client==-1){
                perror("accept() failed");
                // continue;
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
                    fds[client_count+1].fd=client;
                    fds[client_count+1].events = POLLIN;

                    clients[client_count+1].is_login=0;
                    clients[client_count+1].login_failed_count=0;
                    strcpy(clients[client_count+1].password, "");
                    strcpy(clients[client_count+1].username, "");    

                    char msg[256];
                    get_time_to_buf(msg);
                    strcat(msg, " Enter username: ");
                    send(client, msg, strlen(msg), 0);
                    client_count++;
                }
            }
        }


        for (int i=1;i<=client_count;i++){
            if (fds[i].revents & POLLIN){
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));

                ret = recv(fds[i].fd, buffer, sizeof(buffer)-1, 0);
                if (ret <= -1){
                    perror("recv() failed");
                    close(fds[i].fd);
                    if (i<client_count){
                        fds[i] = fds[client_count];
                        clients[i]=clients[client_count];
                    }
                    clients[client_count].is_login=0;
                    clients[client_count].login_failed_count=0;
                    strcpy(clients[client_count].password, "");
                    strcpy(clients[client_count].username, "");                    
                    client_count--;
                    i--;
                } else if (ret==0){
                    perror("client disconnected");
                    close(fds[i].fd);
                    if (i<client_count){
                        fds[i] = fds[client_count];
                        clients[i]=clients[client_count];
                    }
                    clients[client_count].is_login=0;
                    clients[client_count].login_failed_count=0;
                    strcpy(clients[client_count].password, "");
                    strcpy(clients[client_count].username, "");                    
                    client_count--;
                    i--;
                } else {
                    buffer[ret] = 0;
                    printf("Received from %d: %s\n", fds[i].fd, buffer);
                    
                    if (strlen(clients[i].username)==0){
                        
                        char username[32], tmp[32];
                        ret = sscanf(buffer, "%s%s", username, tmp);
                        if (ret==1){
                            strcpy(clients[i].username, username);
                            char msg[256];
                            get_time_to_buf(msg);
                            strcat(msg, " Enter password: ");
                            send(fds[i].fd, msg, strlen(msg), 0);
                        } else {
                            char msg[256];
                            get_time_to_buf(msg);
                            strcat(msg, " Wrong usage. Username must not have space.\nEnter username: ");
                            send(fds[i].fd, msg, strlen(msg), 0);
                        }
                    } else if (strlen(clients[i].password)==0){
                        char password[32], tmp[32];
                        ret = sscanf(buffer, "%s%s%s", password, tmp);
                        if (ret==1){
                            strcpy(clients[i].password, password);
                            
                            if (authenticate(&clients[i])){
                                clients[i].is_login=1;
                                char msg[256];
                                get_time_to_buf(msg);
                                strcat(msg, " Login successful.\n");
                                send(fds[i].fd, msg, strlen(msg), 0);
                            } else {
                                if (clients[i].login_failed_count<3){
                                    clients[i].login_failed_count++;
                                    clients[client_count].is_login=0;
                                    printf("client %d fail login %d time.\n", fds[i].fd, clients[i].login_failed_count);
                                    strcpy(clients[i].password, "");
                                    strcpy(clients[i].username, "");   
                                    char msg[256];
                                    get_time_to_buf(msg);
                                    strcat(msg, " Login failed.\nEnter username: ");
                                    send(fds[i].fd, msg, strlen(msg), 0);
                                } else {
                                    printf("client %d fail login %d time. close client %d.\n", fds[i].fd, clients[i].login_failed_count, fds[i].fd);
                                    char msg[256];
                                    get_time_to_buf(msg);
                                    strcat(msg, " Login failed 3 time. Comeback later.\n");
                                    send(fds[i].fd, msg, strlen(msg), 0);
                                    close(fds[i].fd);
                                    if (i<client_count){
                                        fds[i] = fds[client_count];
                                        clients[i]=clients[client_count];
                                    }
                                    clients[client_count].is_login=0;
                                    clients[client_count].login_failed_count=0;
                                    strcpy(clients[client_count].password, "");
                                    strcpy(clients[client_count].username, "");                    
                                    client_count--;
                                    i--;
                                }
                            }
                        } else {
                            char msg[256];
                            get_time_to_buf(msg);
                            strcat(msg, " Wrong usage. Password must not have space.\nEnter password: ");
                            send(fds[i].fd, msg, strlen(msg), 0);
                        }
                    } else if (clients[i].is_login){
                        // Execute command and send result back to the client
                        char cmd[256];
                        strcpy(cmd, buffer);
                        strcat(cmd, " > out.txt");
                        system(cmd);
                        FILE *result = fopen("out.txt", "r");
                        int ret = 0;
                        char sendbuf[1024];
                        while(!feof(result)){
                            ret = fread(sendbuf, 1, sizeof(sendbuf), result);
                            if (ret<=0){
                                break;
                            }
                            send(fds[i].fd, sendbuf, strlen(sendbuf), 0);
                        }
                        fclose(result);
                    }
                }
            }
        }

    }
    // Close all client connections
    for (int i = 1; i <= client_count; i++) {
        close(fds[i].fd);
    }

    // Close the listener socket
    close(listener);

    return 0;
}