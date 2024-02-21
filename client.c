#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define BUFFER_SIZE 1024
#define PORT 80
#define ADDR "127.0.0.1"
int calcDecodeLength(const char *b64input) {
    int len = strlen(b64input), padding = 0;

    if (b64input[len - 1] == '=' && b64input[len - 2] == '=') //last two chars are =
        padding = 2;
    else if (b64input[len - 1] == '=') //last char is =
        padding = 1;

    return (int)len * 0.75 - padding;
}

int Base64Decode(char *b64message, char **buffer, int length) {
    BIO *bio, *b64;
    int decodeLen = calcDecodeLength(b64message);

    *buffer = (unsigned char*)malloc(decodeLen + 1);
    if (*buffer == NULL) {
        return -1; // Memory allocation failed
    }

    FILE *stream = fmemopen(b64message, strlen(b64message), "r");
    if (stream == NULL) {
        free(*buffer);
        return -1; // File stream open failed
    }

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer

    int len = BIO_read(bio, *buffer, strlen(b64message));
    if (len != decodeLen) {
        free(*buffer);
        BIO_free_all(bio);
        fclose(stream);
        return -1; // Decode length mismatch
    }

    (*buffer)[len] = '\0'; // Null-terminator for text data; not required for binary data
    length = len; // Return the length of the decoded data

    BIO_free_all(bio);
    fclose(stream);

    return 0; // Success
}

void send_get_request(int socket, char *remote_path)
{
    char request[BUFFER_SIZE];
    char size[BUFFER_SIZE];
    memset(request, 0, BUFFER_SIZE);
    sprintf(request, "GET %s\r\n\r\n", remote_path);
    sprintf(size, "%ld", strlen(request));
    sleep(0.5);
    send(socket, size, BUFFER_SIZE, 0);
    sleep(0.5);
    send(socket, request, strlen(request), 0);
}

// void send_post_request(int socket, char *file_path)
// {
//     FILE *file = fopen(file_path, "rb");
//     if (file == NULL)
//     {
//         perror("Error opening file for reading");
//         return;
//     }

//     char base64_contents[BUFFER_SIZE];
//     size_t bytesRead;

//     while ((bytesRead = fread(base64_contents, 1, BUFFER_SIZE, file)) > 0)
//     {
//         // Base64 encode the file contents
//         BIO *b64 = BIO_new(BIO_f_base64());
//         BIO *bmem = BIO_new(BIO_s_mem());
//         b64 = BIO_push(b64, bmem);
//         BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
//         BIO_write(b64, base64_contents, bytesRead);
//         BIO_flush(b64);

//         // Read the base64 encoded data from the BIO
//         int encodedBytes = BIO_read(bmem, base64_contents, BUFFER_SIZE);

//         // Send the base64 encoded data to the server
//         send(socket, base64_contents, encodedBytes, 0);

//         BIO_free_all(b64);
//     }

//     fclose(file);

//     // Send the end of transmission marker
//     send(socket, "\r\n\r\n", strlen("\r\n\r\n"), 0);
// }

int main(int argc, char *argv[])
{
    if (argc != 3 || (strcmp(argv[1], "GET") != 0 && strcmp(argv[1], "POST") != 0))
    {
        fprintf(stderr, "Usage: %s <GET/POST> <remote_path>\n", argv[0]);
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

    if (strcmp(argv[1], "GET") == 0)
    {
        send_get_request(sock, argv[2]);
        char size[BUFFER_SIZE];
        sleep(1);
        recv(sock, size, BUFFER_SIZE, 0);
        printf("the size is:%s\n", size);
        int size_int = atoi(size);
        printf("the size is:%d\n", size_int);
        sleep(1);
        char response[size_int];
        memset(response, 0, size_int);
        recv(sock, response, size_int, 0);
        char code[4];
        strncpy(code, response, 3);
        code[3] = '\0';
        printf("%s\n", code);
        if (strcmp(code, "200") == 0)
        {
            const char *start, *end;
            start = strstr(response, "\r\n");
            start += 2;
            end = strstr(start, "\r\n");
            char base64Content[end-start+1];
            strncpy(base64Content, start, end - start);
            base64Content[end - start] = '\0';
            for (int i = 0; i < strlen(response); i++)
            {
                printf("%02x", (unsigned char)response[i]);
            }
            char *decode_data;
            Base64Decode(base64Content,&decode_data, size_int);
            FILE *res = fopen(argv[2], "wb");
            fwrite(decode_data, 1, strlen(decode_data), res);
            fclose(res);
        }
    }
    // else if (strcmp(argv[1], "POST") == 0)
    // {
    //     if (argc != 3)
    //     {
    //         fprintf(stderr, "Usage: %s POST <remote_path>\n", argv[0]);
    //         exit(EXIT_FAILURE);
    //     }
    //     send_post_request(sock, argv[2]);

    //     printf("File sent successfully.\n");
    // }

    // Close socket
    close(sock);

    return 0;
}
