#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "server.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: $ ./client <dir>");
        exit(EXIT_FAILURE);
    }

    int sock = -1;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0)
    {
        perror("socket failed");
        exit(errno);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ADDR, &server_addr.sin_addr) < 0)
    {
        perror("inet_pton failed");
        close(sock);
        exit(errno);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sock);
        exit(errno);
    }

    printf("connected to server\n");

    pget buffer = NULL;
    buffer = (pget)calloc(1, sizeof(get));
    if (!buffer)
    {
        perror("calloc failed");
        close(sock);
        exit(errno);
    }
    strcpy(buffer->get_str, "GET\0");
    strcpy(buffer->crlf, "\r\n\0");

    buffer->remote_path = (char *)calloc(strlen(argv[1]) + 1, sizeof(char));
    if (!buffer->remote_path)
    {
        perror("calloc failed");
        close(sock);
        exit(errno);
    }
    strcpy(buffer->remote_path, (const char *)argv[1]);

    ssize_t bytes_sent = 0;
    bytes_sent = send(sock, buffer->get_str, sizeof(buffer->get_str), 0);
    if (bytes_sent < 0)
    {
        perror("send failed");
        close(sock);
        exit(errno);
    }

    bytes_sent = 0;
    bytes_sent = send(sock, buffer->remote_path, sizeof(buffer->remote_path), 0);
    if (bytes_sent < 0)
    {
        perror("send failed");
        close(sock);
        exit(errno);
    }

    bytes_sent = 0;
    bytes_sent = send(sock, buffer->crlf, sizeof(buffer->crlf), 0);
    if (bytes_sent < 0)
    {
        perror("send failed");
        close(sock);
        exit(errno);
    }


    // ssize_t bytes_sent = 0;
    // bytes_sent = send(sock, buffer, sizeof(buffer), 0);
    // if (bytes_sent < 0)
    // {
    //     perror("send failed");
    //     close(sock);
    //     exit(errno);
    // }

    free(buffer->remote_path);
    buffer->remote_path = NULL;

    free(buffer);
    buffer = NULL;

    // memset(buffer, 0, BUFSIZ);
    // ssize_t bytes_recv = 0;
    // bytes_recv = recv(sock, buffer, BUFSIZ, 0);
    // if(bytes_recv < 0)
    // {
    //     perror("recv failed");
    //     close(sock);
    //     exit(errno);
    // }
    // else if(bytes_recv == 0)
    // {
    //     close(sock);
    //     return 0;
    // }

    // printf("got response from server: %s\n", buffer);

    close(sock);

    return 0;
}