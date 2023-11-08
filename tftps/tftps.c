#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

#include "tftp.h"

typedef struct
{
    int port;
    int sock_fd;
    struct sockaddr_in laddr;
    int secure;
} context;

typedef enum
{
    STATE_WAIT,
    STATE_RRQ,
    STATE_WRQ,
} e_state;

void show_help()
{
    printf("\n");
}

int main(int argc, char *argv[])
{
    context ctx = {
        .port = 69,
        .sock_fd = -1,
        .secure = 0,
    };

    int ch;
    while ((ch = getopt(argc, argv, "sp:")) != -1)
    {
        switch (ch)
        {
        case 's':
            ctx.secure = 1;
            break;
        case 'p':
            ctx.port = atoi(optarg);
            break;
        case 'h':
        default:
            show_help();
            exit(0);
        }
    }

    if ((ctx.sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("socket failed");
        exit(-1);
    }

    ctx.laddr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port = htons(ctx.port),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };

    if (bind(ctx.sock_fd, (struct sockaddr *)&ctx.laddr, sizeof(ctx.laddr)) < 0)
    {
        printf("bind failed: %s\n", strerror(errno));
        exit(-1);
    }

    e_state state = STATE_WAIT;
    char packet_buf[PACKET_SIZE_] = {0};
    while (1)
    {
        // recv data
        memset(packet_buf, 0, PACKET_SIZE_);
        struct sockaddr_in raddr;
        socklen_t raddr_len;
        int n = recvfrom(ctx.sock_fd, packet_buf, PACKET_SIZE_, 0, (struct sockaddr *)&raddr, &raddr_len);
        if (n < 0)
        {
            printf("recvfrom failed: %s\n", strerror(errno));
            continue;
        }
        // parse data
        struct tftphdr *tp = (struct tftphdr *)packet_buf;
        switch (ntohs(tp->th_opcode))
        {
        case RRQ:
            if (state != STATE_WAIT)
            {
                
            }
        case WRQ:
        case ACK:
        }
    }
}