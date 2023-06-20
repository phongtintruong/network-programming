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

int client_id[100][3]; // [0] is socket, [1] is client's state, [2] is client's friend socket id
int queue_state; // 0 means no one in queue, other means client socket id
char *client_name[100];
int client_count=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
        client_id[i][2] = -1; // -1 means no friend
    }
    queue_state = 0;

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
                for (int i=1;i<=client_count;i++){
                    if (client_id[i][1]==-1){
                        check=1;
                        printf("New user connected: %d\n", client);
                        pthread_t thread_id;
                        pthread_mutex_lock(&mutex);
                        client_id[i][0] = client;
                        client_id[i][1] = 0; // 0 means not login
                        pthread_mutex_unlock(&mutex);
                        if (queue_state==0){
                            pthread_mutex_lock(&mutex);
                            queue_state = i;
                            pthread_mutex_unlock(&mutex);
                        } else {
                            pthread_mutex_lock(&mutex);
                            client_id[i][2] = queue_state;
                            client_id[queue_state][2] = i;
                            queue_state = 0;
                            pthread_mutex_unlock(&mutex);
                        }
                        
                        // printf("start thread %d with queue_state %d\n", client, queue_state);
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
                client_id[client_count][0] = client;
                client_id[client_count][1] = 0; // 0 means not login
                pthread_mutex_unlock(&mutex);
                int client_index = client_count;
                if (queue_state==0){
                    // printf("test 6\n");
                    pthread_mutex_lock(&mutex);
                    queue_state = client_index;
                    // printf("test 6.5: %d\n", queue_state);
                    pthread_mutex_unlock(&mutex);
                } else {
                    // printf("test 7: %d\n", queue_state);
                    pthread_mutex_lock(&mutex);
                    client_id[client_index][2] = queue_state;
                    client_id[queue_state][2] = client_index;
                    queue_state = 0;
                    pthread_mutex_unlock(&mutex);
                }
                // printf("start thread %d with queue_state %d\n", client, queue_state);
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

    int friend_index = client_id[client_index][2];
    int friend = client_id[friend_index][0];

    char buf[256];
    int is_first_time = 1;

    char msg[256];
    get_time_to_buf(msg);
    strcat(msg, " Please login. Usage: client_id: <id>.\n");
    send(client, msg, strlen(msg), 0);
    while (client_id[client_index][0]!=-1){

        int ret = recv(client, buf, sizeof(buf), 0);
        if (ret<=-1){
            perror("recv() failed");
            close(client);
            close(friend);

            pthread_mutex_lock(&mutex);
            queue_state=0;

            client_id[client_index][0] = -1;
            client_id[client_index][1] = -1;
            client_id[client_index][2] = -1;
            
            client_id[friend_index][0] = -1;
            client_id[friend_index][1] = -1;
            client_id[friend_index][2] = -1;
            if (client_name[client_index]!=NULL){
                free(client_name[client_index]);
            }
            if (client_name[friend_index]!=NULL){
                free(client_name[friend_index]);
            }
            pthread_mutex_unlock(&mutex);
            break;
        } else if (ret==0){
            perror("client disconnected");
            close(client);
            close(friend);

            pthread_mutex_lock(&mutex);
            
            client_id[client_index][0] = -1;
            client_id[client_index][1] = -1;
            client_id[client_index][2] = -1;
            
            client_id[friend_index][0] = -1;
            client_id[friend_index][1] = -1;
            client_id[friend_index][2] = -1;

            if (client_name[client_index]!=NULL){
                free(client_name[client_index]);
            }
            if (client_name[friend_index]!=NULL){
                free(client_name[friend_index]);
            }
            pthread_mutex_unlock(&mutex);
            // printf("test disconnect\n");
            break;
        } else {
            buf[ret-1] = 0;
            printf("Received from %d: %s\n", client, buf);
            if (client_id[client_index][1]==0){
                // printf("test 1\n");
                char cmd[32], id[32], tmp[32];
                ret = sscanf(buf, "%s %s %s", cmd, id, tmp);
                if (ret==2){
                    // printf("test 2\n");
                    if (strcmp(cmd, "client_id:")==0){
                        // printf("test 3\n");
                        char msg[256];
                        get_time_to_buf(msg);
                        strcat(msg, " You are signed in. Enter message.\n");
                        send(client, msg, strlen(msg), 0);

                        client_id[client_index][1] = 1; // 1 means login
                        client_name[client_index] = malloc(strlen(id)+1);
                        strcpy(client_name[client_index], id);
                    } else {
                        // printf("test 4\n");
                        char msg[256];
                        get_time_to_buf(msg);
                        strcat(msg, " Wrong usage. Usage: client_id: <id>.\n");
                        send(client, msg, strlen(msg), 0);
                    }
                } else {
                    // printf("test 5\n");
                    char msg[256];
                    get_time_to_buf(msg);
                    strcat(msg, " Wrong usage. Usage: client_id: <id>.\n");
                    send(client, msg, strlen(msg), 0);
                }
            } else {
                friend_index = client_id[client_index][2];
                friend = client_id[friend_index][0];
                // printf("test friend_index = %d\n", friend_index);
                // send process 
                if (friend_index!=-1){
                
                    char sendbuf[256];

                    get_time_to_buf(sendbuf);
                    strcat(sendbuf, " ");
                    strcat(sendbuf, client_name[client_index]);
                    strcat(sendbuf, ": ");
                    strcat(sendbuf, buf);
                    strcat(sendbuf, "\n");

                    // printf("friend_index = %d\n", friend_index);
                    send(friend, sendbuf, strlen(sendbuf), 0);
                } else if (is_first_time){
                    // printf("friend_index = %d\n", friend_index);
                    char msg[256];
                    get_time_to_buf(msg);
                    strcat(msg, " Waiting for friend to connect.\n");
                    send(client, msg, strlen(msg), 0);
                    is_first_time = 0;
                }
            }
        }
    }
    get_time_to_buf(msg);
    strcat(msg, " Chat room closed.\n");
    send(client, msg, strlen(msg), 0);
    close(client);
    close(friend);
}
