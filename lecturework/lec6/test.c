#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(){
    printf("Before fork()\n");
    int cid = fork();
    int pid = getpid();
    if (cid<0){
        perror("fork() failed");
        exit(1);
    } else if (cid==0){
        while (1){
            printf("Start\n");
            sleep(1);
            printf("Child => cid: %d and pid: %d\n", cid, pid);
            // exit(0);
        }
    } else {
        printf("Parent => cid: %d and pid: %d\n", cid, pid);  
    }
    return 0;
}