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
        fprintf(stderr, "Usage: $ sudo ./server <root dir>");
        exit(EXIT_FAILURE);
    }

    int sock = -1;
    sock = socket(AF_INET, SOCK_STREAM, 0);
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

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sock);
        exit(errno);
    }

    if (listen(sock, 1) < 0)
    {
        perror("listen failed");
        close(sock);
        exit(errno);
    }
    printf("listening on: %s:%d\n", ADDR, PORT);

    int client = -1;
    client = accept(sock, NULL, NULL);
    if (client < 0)
    {
        perror("accept failed");
        close(sock);
        exit(errno);
    }
    printf("client connected\n");

    char buffer[BUFSIZ];
    memset(buffer, 0, BUFSIZ);
    ssize_t bytes_recv = 0;
    bytes_recv = recv(client, buffer, BUFSIZ, 0);
    if (bytes_recv < 0)
    {
        perror("recv failed");
        close(client);
        close(sock);
        exit(errno);
    }
    else if (bytes_recv == 0)
    {
        close(client);
        close(sock);
        return 0;
    }
    
    pget request = (pget)buffer;
    printf("remote_path: %s\n", request->remote_path);
    // printf("method: %s\nremote_path: %s\ncrlf: %s", request->get_str, request->remote_path, request->crlf);

    // printf("received: %s\n", buffer);

    // // procerssing
    // memset(buffer, '1', BUFSIZ);

    // ssize_t bytes_sent = 0;
    // bytes_sent = send(client, buffer, BUFSIZ, 0);
    // if(bytes_sent < 0)
    // {
    //     perror("recv failed");
    //     close(client);
    //     close(sock);
    //     exit(errno);
    // }

    // printf("ans was sent to client\n");

    close(client);
    close(sock);

    return 0;
}