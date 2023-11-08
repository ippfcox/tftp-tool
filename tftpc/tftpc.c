#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

#include "tftp.h"

// [ ] PUT

#define PROMPT_SIZE_ 1024
#define PACKET_SIZE_ ((SEGSIZE) + 4)

typedef struct
{
    char ip[16];              // tftpd ip
    int port;                 // tftpd port
    int sock_fd;              // socket fd
    struct sockaddr_in raddr; // tftpd addr struct
    int connected;            // connection status
} context;

// do connect, create socket and assign raddr
int do_connect(context *ctx)
{
    if ((ctx->sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        printf("socket failed");
        return -1;
    }

    ctx->raddr = (struct sockaddr_in){
        .sin_family = AF_INET,
        .sin_port = htons(ctx->port),
        .sin_addr.s_addr = inet_addr(ctx->ip),
    };

    ctx->connected = 1;

    return 0;
}

// do disconnect, close socket if opened
void do_disconnect(context *ctx)
{
    if (ctx->sock_fd > 0)
    {
        close(ctx->sock_fd);
        ctx->sock_fd = -1;
    }

    ctx->connected = 0;
}

// do get, send RRQ and write file
int do_get(context *ctx, char *filename, char *mode)
{
    // open file
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        printf("fopen failed: %s\n", strerror(errno));
        return errno;
    }
    // make RRQ
    char packet_buf[PACKET_SIZE_] = {0};
    struct tftphdr *tp = (struct tftphdr *)packet_buf;
    tp->th_opcode = htons(RRQ);
    char *cp = (char *)&(tp->th_stuff);
    strncpy(cp, filename, strlen(filename));
    cp += strlen(filename);
    *cp++ = '\0';
    strncpy(cp, mode, strlen(mode));
    cp += strlen(mode);
    *cp++ = '\0';
    // send RRQ
    if (sendto(ctx->sock_fd, packet_buf, PACKET_SIZE_, 0, (struct sockaddr *)&ctx->raddr, sizeof(ctx->raddr)) < 0)
    {
        printf("sendto failed: %s\n", strerror(errno));
        return errno;
    }
    // recv data and write file
    int finish = 0;
    while (!finish)
    {
        // recv data
        memset(packet_buf, 0, PACKET_SIZE_);
        struct sockaddr_in raddr;
        socklen_t raddr_len;
        int n = recvfrom(ctx->sock_fd, packet_buf, PACKET_SIZE_, 0, (struct sockaddr *)&raddr, &raddr_len);
        if (n < 0)
        {
            printf("recvfrom: %s\n", strerror(errno));
            continue;
        }
        // parse data
        struct tftphdr *tp = (struct tftphdr *)packet_buf;
        switch (ntohs(tp->th_opcode))
        {
        case DATA:
        {
            // write file
            fwrite(tp->th_data, 1, n - 4, fp);
            // send ACK
            struct tftphdr hdr = {.th_opcode = htons(ACK), .th_block = tp->th_block};
            sendto(ctx->sock_fd, &hdr, sizeof(struct tftphdr), 0, (struct sockaddr *)&raddr, raddr_len); // [TODO) check
            if (n < PACKET_SIZE_)
                finish = 1;
            break;
        }
        case ERROR:
            printf("error(server): %s(%d)\n", tp->th_msg, ntohs(tp->th_code));
            finish = 1;
            break;
        default:
            printf("unknown opcode: %d", ntohs(tp->th_opcode));
            break;
        }
    }
    // close file
    fclose(fp);
    return 0;
}

// str trim: [   abc   xyz   ] -> [abc xyz]
char *str_trim_space(char *s)
{
    char *p = s;
    int space = 1;
    for (char *c = s; *c != '\0'; c++)
    {
        if (space != 1 || (*c != ' ' && *c != '\t' && *c != '\v' && *c != '\f' && *c != '\r'))
            *p++ = *c;
        space = *c == ' ' ? 1 : 0;
    }
    *p++ = '\0';
    return s;
}

// str to lower
char *str_to_lower(char *s)
{
    for (char *c = s; *c != '\0'; c++)
        *c = tolower(*c);
    return s;
}

// show help
void show_help()
{
    printf("\
    connect <ip>[:port]: connect to remote server, default port: 69\n\
    disconnect:          disconnect from remote server\n\
    get <filename>:      download file from server\n\
    put <filename>:      upload file to server\n\
    help:                show this help\n");
}

int main()
{
    context ctx = {
        .ip = "10.1.41.45",
        .port = 69,
        .sock_fd = -1,
        .connected = 0,
    };

    char prompt[PROMPT_SIZE_] = {0};

    while (1)
    {
        if (ctx.connected)
            fprintf(stdout, "\033[32mtftp %s:%d> \033[0m", ctx.ip, ctx.port);
        else
            fprintf(stdout, "\033[32mtftp> \033[0m");
        // get prompt
        memset(prompt, 0, PROMPT_SIZE_);
        fgets(prompt, PROMPT_SIZE_ - 1, stdin);
        // parse cmd and data
        char cmd[256] = {0}, data[512] = {0};
        sscanf(str_trim_space(prompt), "%s %s", cmd, data);

        if (strcmp(str_to_lower(cmd), "") == 0)
        {
            continue;
        }
        else if (strcmp(str_to_lower(cmd), "connect") == 0)
        {
            ctx.port = 69;
            sscanf(data, "%15[^:]:%d", ctx.ip, &ctx.port);

            struct in_addr ipv4;
            if (inet_pton(AF_INET, ctx.ip, &ipv4) != 1)
            {
                printf("invalid ip <%s>\n", ctx.ip);
                continue;
            }

            if (do_connect(&ctx) < 0)
            {
                printf("connect failed\n");
                continue;
            }
        }
        else if (strcmp(str_to_lower(cmd), "disconnect") == 0)
        {
            do_disconnect(&ctx);

            continue;
        }
        else if (strcmp(str_to_lower(cmd), "get") == 0)
        {
            if (!ctx.connected)
            {
                printf("should connect first\n");
                continue;
            }

            if (do_get(&ctx, data, "octet") < 0)
            {
                printf("get failed\n");
                continue;
            }
        }
        else if (strcmp(str_to_lower(cmd), "put") == 0)
        {
            if (!ctx.connected)
            {
                printf("should connect first\n");
                continue;
            }
        }
        else if (strcmp(str_to_lower(cmd), "help") == 0)
        {
            show_help();
            continue;
        }
        else
        {
            printf("unknown command: %s\n", cmd);
            show_help();
            continue;
        }
    }
}
