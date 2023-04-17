#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(){
    char str[] = "227 Entering Passive Mode (213,229,112,130,216,4)";

    int ip[4];
    int port[2];

    char *pc = strchr(str, '(');
    char *p = strtok(pc, "(),");
    // p = strtok(NULL, "(,)");

    int j=0;
    while (p != NULL){
        if (j<4){
            ip[j]=atoi(p);
            // printf("%u\n", ip[j]);
            p = strtok(NULL, "(),");
        }
        else {
            port[j-4]=atoi(p);
            // printf("%d: %d",j-4, port[j-4]);
            p = strtok(NULL, "(,)");
        }
        j++;
    }
    printf("IP: %u.%u.%u.%u\nPort: %u", ip[0], ip[1], ip[2], ip[3], port[0]*256+port[1]);
    return 0;
}