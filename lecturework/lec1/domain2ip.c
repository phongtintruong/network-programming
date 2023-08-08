#include <stdio.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <string.h>

int main(int argc, char *argv[]){
    if (argc != 2){
        printf("Usage: %s <domain name>\n", argv[0]);
        return 1;
    }
    struct addrinfo *res, *p;
    int ret = getaddrinfo(argv[1], "http", NULL, &res);
    if (ret != 0){
        printf("getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }
    for (p = res; p != NULL; p = p->ai_next){
        if (p->ai_family == AF_INET){
            printf("IPv4: ");
            struct sockaddr_in addr;
            memcpy(&addr, p->ai_addr, p->ai_addrlen);
            printf("IP: %s\n", inet_ntoa(addr.sin_addr));
        } else if (p->ai_family == AF_INET6){
            printf("IPv6: ");
            struct sockaddr_in6 addr;
            memcpy(&addr, p->ai_addr, p->ai_addrlen);
            char ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &addr.sin6_addr, ip, sizeof(ip));
            printf("IP: %s\n", ip);
        } else {
            printf("Unknown family\n");
        }
    }
    freeaddrinfo(res);
    return 0;
}