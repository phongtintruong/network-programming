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

int client_id[100][3];
char *client_name[100];
int client_count=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int isValidId(char *id){
    pthread_mutex_lock(&mutex);
    int j=0;
    while (client_name[j]!=NULL){
        if (strcmp(id, client_name[j])==0){
            pthread_mutex_unlock(&mutex);
            return 200;
        }
        j++;
    }
    for (int i=0;i<strlen(id);i++){
        if (!((id[i]>='a'&&id[i]<='z')||(id[i]>='0'&&id[i]<='9'))){
            pthread_mutex_unlock(&mutex);
            return 201;
        }
    }
    pthread_mutex_unlock(&mutex);
    return 100;
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

void *client_thread(void *arg);

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

    for (int i=0;i<100;i++){
        client_id[i][0] = -1;
        client_id[i][1] = -1; // -1 means not connected
        client_id[i][2] = -1; // -1 means not login, 0 means normal user, 1 means admin
    }

    printf("Server started. Waiting for connections...\n");


    while (1){
        int client = accept(listener, NULL, NULL);
        if (client==-1){
            perror("accept() failed");
        } else {
            if (client_count>max_noclient){
                char msg[256];
                get_time_to_buf(msg);
                strcat(msg, " number of clients limit exceeds, comeback later.\n");
                send(client, msg, strlen(msg), 0);
                printf("number of clients limit exceeds, excess client will not be processed\n");
                close(client);
            } else if (client_count==max_noclient){
                int check=0;
                for (int i=0;i<client_count;i++){
                    if (client_id[i][1]==-1){
                        check=1;
                        printf("New user connected: %d\n", client);
                        pthread_t thread_id;
                        pthread_mutex_lock(&mutex);
                        client_id[i][0] = client;
                        client_id[i][1] = 0; // 0 means not login
                        pthread_mutex_unlock(&mutex);
                        printf("start thread %d\n", client);
                        if(pthread_create(&thread_id, NULL, client_thread, (void *)&i)!=0){
                            perror("pthread_create() failed");
                            return 1;
                        }
                        
                        pthread_detach(thread_id);
                        break;
                    }
                }
                if (check==0){
                    char msg[256];
                    get_time_to_buf(msg);
                    strcat(msg, " number of clients limit exceeds, comeback later.\n");
                    send(client, msg, strlen(msg), 0);
                    printf("number of clients limit exceeds, excess client will not be processed\n");
                    close(client);
                }            
            } else {
                printf("New user connected: %d\n", client);
                pthread_t thread_id;
                pthread_mutex_lock(&mutex);
                client_count++;
                client_id[client_count-1][0] = client;
                client_id[client_count-1][1] = 0; // 0 means not login
                pthread_mutex_unlock(&mutex);
                int client_index = client_count-1;
                printf("start thread %d\n", client);
                if(pthread_create(&thread_id, NULL, client_thread, (void *)&client_index)!=0){
                    perror("pthread_create() failed");
                    return 1;
                }
                
                pthread_detach(thread_id);
            }
        }
    }
    close(listener);
    return 0;    
}

void *client_thread(void *param){
    int client_index = *(int *)param;
    int client = client_id[client_index][0];
    char buf[256];

    while (1){
        int ret = recv(client, buf, sizeof(buf), 0);
        if (ret<=-1){
            perror("recv() failed");
            char msg[256];
            // get_time_to_buf(msg);
            strcpy(msg, "");
            strcat(msg, "999 UNKNOWN ERROR\n");
            send(client, msg, strlen(msg), 0);
            close(client);

            pthread_mutex_lock(&mutex);
            client_id[client_index][0] = -1;
            client_id[client_index][1] = -1;
            client_id[client_index][2] = -1;
            free(client_name[client_index]);
            

            if (client_id[client_index][2]==1){
                for (int i=0;i<client_count;i++){
                    if (client_id[i][1]==1){
                        client_id[i][2] = 1;
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&mutex);
            break;
        } else if (ret==0){
            perror("client disconnected");
            char msg[256];
            // get_time_to_buf(msg);
            strcpy(msg, "");
            strcat(msg, "999 UNKNOWN ERROR\n");
            send(client, msg, strlen(msg), 0);
            close(client);

            pthread_mutex_lock(&mutex);
            client_id[client_index][0] = -1;
            client_id[client_index][1] = -1;
            client_id[client_index][2] = -1;
            free(client_name[client_index]);
            

            if (client_id[client_index][2]==1){
                for (int i=0;i<client_count;i++){
                    if (client_id[i][1]==1){
                        client_id[i][2] = 1;
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&mutex);
            break;
        } else {
            buf[ret] = 0;
            printf("Received from %d: %s\n", client, buf);
            if (client_id[client_index][1]==0){
                char cmd[32], id[32], tmp[32];
                ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
                if (ret==2){
                    if (strcmp(cmd, "JOIN")==0){
                        if (isValidId(id)==100){
                            char msg[256];
                            // get_time_to_buf(msg);
                            strcpy(msg, "");
                            strcat(msg, "100 OK\n");
                            send(client, msg, strlen(msg), 0);

                            pthread_mutex_lock(&mutex);
                            int login_count = 0;
                            for (int i=0;i<client_count;i++){
                                if (client_id[i][1]==1){
                                    login_count++;
                                }
                            }

                            client_id[client_index][1] = 1; // 1 means login
                            client_name[client_index] = malloc(strlen(id)+1);
                            strcpy(client_name[client_index], id);

                            if (login_count==0){
                                client_id[client_index][2] = 1; // 1 means admin
                            }
                            pthread_mutex_unlock(&mutex);
                        } else if (isValidId(id)==200){
                            char msg[256];
                            // get_time_to_buf(msg);
                            strcpy(msg, "");
                            strcat(msg, "200 NICKNAME IN USE\n");
                            send(client, msg, strlen(msg), 0);
                        } else if (isValidId(id)==201){
                            char msg[256];
                            // get_time_to_buf(msg);
                            strcpy(msg, "");
                            strcat(msg, "201 INVALID NICK NAME\n");
                            send(client, msg, strlen(msg), 0);
                        } else {
                            char msg[256];
                            // get_time_to_buf(msg);
                            strcpy(msg, "");
                            strcat(msg, "999 UNKNOWN ERROR\n");
                            send(client, msg, strlen(msg), 0);
                        }
                    } else {
                        char msg[256];
                        // get_time_to_buf(msg);
                        strcpy(msg, "");
                        strcat(msg, "000 NOT LOGIN\n");
                        send(client, msg, strlen(msg), 0);
                    }
                } else {
                    if (strcmp(cmd, "JOIN")==0){
                        char msg[256];
                        // get_time_to_buf(msg);
                        strcpy(msg, "");
                        strcat(msg, "201 INVALID NICK NAME\n");
                        send(client, msg, strlen(msg), 0);
                    } else {
                        char msg[256];
                        // get_time_to_buf(msg);
                        strcpy(msg, "");
                        strcat(msg, "000 NOT LOGIN\n");
                        send(client, msg, strlen(msg), 0);
                    }
                }
            } else {
                char sendbuf[256];

                get_time_to_buf(sendbuf);
                strcat(sendbuf, " ");
                strcat(sendbuf, client_name[client_index]);
                strcat(sendbuf, ": ");
                strcat(sendbuf, buf);

                for (int j=0;j<client_count;j++){
                    if (client_id[j][0]!=client_id[client_index][0]&&client_id[j][1]==1){
                        send(client_id[j][0], sendbuf, strlen(sendbuf), 0);
                    }
                }
            }
        }
    }
    close(client);
}
