#include <stdio.h>

int main(){
    char packet_bytes[] = {
        0x45, 0x00, 0x00, 0x40, 
        0x7c, 0xda, 0x40, 0x00,
        0x80, 0x06, 0xfa, 0xd8, 
        0xc0, 0xa8, 0x0e, 0x0a,
        0xbc, 0xac, 0xf6, 0xa4
    };

    char b = packet_bytes[0];
    char ver = b>>4;
    printf("version: %d\n", ver);

    char ihl = (b*0x0f)*4;
    printf("IHL = %d\n", ihl);

    unsigned int tl = packet_bytes[2]*256 +packet_bytes[3];
    printf("TL = %d\n",tl);

    printf("IP nguon: %u.%u.%u.%u\n",
        (unsigned char)packet_bytes[12],
        (unsigned char)packet_bytes[13],
        (unsigned char)packet_bytes[14],
        (unsigned char)packet_bytes[15]
        );

    printf("IP dich: %u.%u.%u.%u\n",
        (unsigned char)packet_bytes[16],
        (unsigned char)packet_bytes[17],
        (unsigned char)packet_bytes[18],
        (unsigned char)packet_bytes[19]
        );
}