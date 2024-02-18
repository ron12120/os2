#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <sys/time.h>

#define PORT 80
#define ADDR "127.0.0.1"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: $ ./client <dir>");
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

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sock);
        exit(errno);
    }

    printf("connected to server\n");

    char choice[32];
    ssize_t bytes_recv = 0;
    bytes_recv = recv(sock, choice, 32, 0); // recving the options from server
    if (bytes_recv < 0)
    {
        perror("recv failed");
        close(sock);
        exit(errno);
    }
    printf("%s\n", choice);
    char c[1];
    memset(c, 0, 1);
    scanf("%s", c);
    ssize_t bytes_sent = 0;
    bytes_sent = send(sock, c, sizeof(c), 0); // sending the choice
    if (bytes_sent < 0)
    {
        perror("send failed");
        close(sock);
        exit(errno);
    }

    bytes_sent = 0;
    bytes_sent = send(sock, argv[1], strlen(argv[1]) + 1, 0); // sending the remote path of the client
    if (bytes_sent < 0)
    {
        perror("send failed");
        close(sock);
        exit(errno);
    }

    if (!strcmp(c, "1"))
    {
        char size[1024];
        memset(size, 0, 1024);
        bytes_recv = 0;
        bytes_recv = recv(sock, size, 1024, 0); // recving the size of the response
        int size_res = atoi(size);
        char res[size_res];
        memset(res, 0, size_res);
        bytes_recv = 0;
        bytes_recv = recv(sock, res, size_res, 0);
        printf("%s", res);
    }
    else if (!strcmp(c, "0"))
    {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL)
        {
            perror("fopen failed8");
            close(sock);
            exit(errno);
        }

        fseek(file, 0, SEEK_END);
        int size = ftell(file);
        rewind(file);
        char buffer[size + 1];
        memset(buffer, 0, size + 1);
        // Read the file into the buffer
        fread(buffer, 1, size, file);
        buffer[size] = '\0'; // Null-terminate the buffer to treat it as a C-string
        fclose(file);        // Close the file as soon as we're done with it
        char file_size[1024];
        memset(file_size, 0, 1024);
        sprintf(file_size, "%d", size + 1);
        sleep(1);
        if (send(sock, file_size, 1024, 0) < 0) // sending the encripted file size
        {
            perror("send size failed");
            close(sock);
            exit(errno);
        }
        sleep(0.5);
        if (send(sock, buffer, size + 1, 0) < 0) // sending the encripted file size
        {
            perror("send data failed");
            close(sock);
            exit(errno);
        }
    }

    close(sock);

    return 0;
}