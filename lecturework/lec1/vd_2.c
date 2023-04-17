#include <stdio.h>
#include <string.h>

int main(){
    char str[256];
    char cmd[6], tmp[6];
    float x, y, ans;

    printf("Enter command: ");
    fgets(str, sizeof(str), stdin);

    int ret = sscanf(str, "%s%f%f%s", cmd, &x, &y, tmp);

    if (ret>3){
        printf("Excess arguments");
        return 1;
    }
    if (ret<3){
        printf("Missing arguments");
        return 1;
    }

    if (cmd!="ADD"){
        ans = x+y;
    }
    else if (cmd!="SUB"){
        ans = x-y;
    }
    else if (cmd!="MUL"){
        ans = x*y;
    }
    else if (cmd!="DIV"){
        ans = x/y;
    }
    else { 
        printf("Error command");
        return 1;
    }
    printf("Valid command!\n %f + %f = %f",x,y,ans);
    return 0;
}