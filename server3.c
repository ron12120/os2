#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <sys/time.h>

#define PORT 80
#define ADDR "127.0.0.1"

struct flock fl = {
    .l_type = F_WRLCK,
    .l_whence = SEEK_SET,
    .l_start = 0,
    .l_len = 0,
};

int lockFile(const char *filepath)
{
    int fd;

    if ((fd = open(filepath, O_RDWR)) == -1)
    {
        perror("open");
        return -1;
    }

    fl.l_type = F_RDLCK;

    if (fcntl(fd, F_SETLKW, &fl) == -1)
    {
        perror("fcntl");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int unlockFile(const char *filepath)
{
    int fd;

    if ((fd = open(filepath, O_RDWR)) == -1)
    {
        perror("open");
        return -1;
    }

    fl.l_type = F_UNLCK;

    if (fcntl(fd, F_SETLK, &fl) == -1)
    {
        perror("fcntl");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int calcDecodeLength(const char *b64input)
{ // Calculates the length of a decoded base64 string
    int len = strlen(b64input);
    int padding = 0;

    if (b64input[len - 1] == '=' && b64input[len - 2] == '=') // last two chars are =
        padding = 2;
    else if (b64input[len - 1] == '=') // last char is =
        padding = 1;

    return (int)len * 0.75 - padding;
}

int Base64Decode(char *b64message, char **buffer)
{ // Decodes a base64 encoded string
    BIO *bio, *b64;
    int decodeLen = calcDecodeLength(b64message);
    int len = 0;
    *buffer = (char *)malloc(decodeLen + 1);
    FILE *stream = fmemopen(b64message, strlen(b64message), "r");

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer
    len = BIO_read(bio, *buffer, strlen(b64message));
    // Can test here if len == decodeLen - if not, then return an error
    (*buffer)[len] = '\0';

    BIO_free_all(bio);
    fclose(stream);

    return (0); // success
}

int Base64Encode(const char *message, char **buffer)
{ // Encodes a string to base64
    BIO *bio, *b64;
    FILE *stream;
    int encodedSize = 4 * ceil((double)strlen(message) / 3);
    *buffer = (char *)malloc(encodedSize + 1);

    stream = fmemopen(*buffer, encodedSize + 1, "w");
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Ignore newlines - write everything in one line
    BIO_write(bio, message, strlen(message));
    BIO_flush(bio);
    BIO_free_all(bio);
    fclose(stream);

    return (0); // success
}

int getFileSize(FILE *file)
{
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    return size;
}

int GET(int client_fd, char *path)
{
    FILE *file;
    unsigned char *buffer;
    size_t file_size;
    char *decoded_data;

    // Open the file
    file = fopen(path, "rb");
    if (file == NULL)
    {
        send(client_fd, "23", 3, 0);
        sleep(0.5);
        send(client_fd, "404 FILE NOT FOUND\r\n\r\n", 23, 0);
        return -1; // Indicate failure
    }

    lockFile(path);

    // Determine file size
    file_size = getFileSize(file);

    // Allocate memory for the file content
    buffer = (unsigned char *)malloc(file_size + 1);
    if (!buffer)
    {
        fclose(file);
        send(client_fd, "23", 3, 0);
        sleep(0.5);
        send(client_fd, "500 INTERNAL ERROR\r\n\r\n", 23, 0);
        return -1; // Indicate failure
    }

    // Read the file into the buffer
    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0'; // Null-terminate the buffer to treat it as a C-string
    fclose(file);             // Close the file as soon as we're done with it

    // Encode the content to Base64
    if (Base64Decode(buffer, &decoded_data) != 0)
    {
        free(buffer); // Make sure to free the buffer if encoding fails
        send(client_fd, "23", 3, 0);
        sleep(0.5);
        send(client_fd, "500 INTERNAL ERROR\r\n\r\n", 23, 0);
        return -1; // Indicate failure
    }

    printf("decoded data %s\n", decoded_data);
    int res_size = strlen(decoded_data) + 14;
    char res_sizeS[1024];
    memset(res_sizeS, 0, res_size);

    sprintf(res_sizeS, "%d", res_size);
    // Free the buffer as it's no longer needed
    free(buffer);
    send(client_fd, res_sizeS, 1024, 0);
    sleep(0.5);
    char response[res_size];
    memset(response, 0, res_size);
    strcat(response, "200 OK\r\n");
    strcat(response, decoded_data);
    strcat(response, "\r\n\r\n");
    // Send the response with the encoded data
    send(client_fd, response, res_size, 0);

    // Free the encoded data after sending it
    free(decoded_data);

    return 0; // Indicate success
}

int POST(int client_fd, char *path)
{
    char *encoded_data;
    ssize_t bytes_recv;
    char size[1024];
    memset(size, 0, 1024);
    bytes_recv = 0;
    sleep(1);
    bytes_recv = recv(client_fd, size, 1024, 0); // recving the size of the encripted file
    if (bytes_recv < 0)
    {
        perror("recv size failed");
        return -1;
    }
    printf("the size is: %s\n", size);
    int size_res = atoi(size);
    char response[size_res];
    memset(response, 0, size_res);
    bytes_recv = 0;
    sleep(0.5);
    bytes_recv = recv(client_fd, response, size_res, 0); // recving the size of the encripted file
    if (bytes_recv < 0)
    {
        perror("recv size failed");
        return -1;
    }
    printf("the response is: %s\n", response);
    if (Base64Encode(response, &encoded_data) != 0)
    {
        return -1; // Indicate failure
    }
    FILE *file;
    file = fopen(path, "w"); // saving the file
    if (file == NULL)
    {
        return -1; // Indicate failure
    }
    // lockFile(path);
    printf("%s\n", encoded_data);
    fwrite(encoded_data, sizeof(char), strlen(encoded_data), file);
    fclose(file);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: $ sudo ./server <root dir>\n");
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

    int on = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt failed");
        close(sock);
        exit(errno);
    }

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        close(sock);
        exit(errno);
    }

    if (listen(sock, 10) < 0)
    {
        perror("listen failed");
        close(sock);
        exit(errno);
    }
    printf("listening on: %s:%d\n", ADDR, PORT);

    int new_socket;
    while (1)
    {
        if ((new_socket = accept(sock, NULL, NULL)) < 0)
        {
            perror("accept");
            close(sock);
            exit(EXIT_FAILURE);
        }
        printf("client connected\n");

        // Forking a new process
        int pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            close(sock);
            close(new_socket);
            exit(EXIT_FAILURE);
        }

        // Child process
        if (pid == 0)
        {
            close(sock);
            char choice[32] = "to POST press 0 to GET press 1\n";
            ssize_t bytes_sent = 0;
            bytes_sent = send(new_socket, choice, 32, 0); // sending the options to client
            if (bytes_sent < 0)
            {
                perror("send failed");
                close(new_socket);
                close(sock);
                exit(errno);
            }
            char c[1];
            ssize_t bytes_recv = 0;
            bytes_recv = recv(new_socket, c, 1, 0); // recving the choice
            if (bytes_recv < 0)
            {
                perror("recv failed");
                close(new_socket);
                close(sock);
                exit(errno);
            }
            int post_get = atoi(c);

            char remote_path_client[1024];
            memset(remote_path_client, 0, 1024);
            bytes_recv = 0;
            bytes_recv = recv(new_socket, remote_path_client, 1024, 0); // recving the remote path from client
            if (bytes_recv < 0)
            {
                perror("recv failed");
                close(new_socket);
                close(sock);
                exit(errno);
            }

            char full_path[2048];
            memset(full_path, 0, 2048);
            strcpy(full_path, argv[1]);
            strcat(full_path, remote_path_client);

            if (post_get == 1)
            {
                if (GET(new_socket, full_path) < 0)
                {
                    perror("GET failed");
                    close(new_socket);
                    close(sock);
                    exit(errno);
                }
                unlockFile(full_path);
            }
            else if (post_get == 0)
            {
                if (POST(new_socket, full_path) < 0)
                {
                    perror("POST failed");
                    close(new_socket);
                    close(sock);
                    exit(errno);
                }
                // unlockFile(full_path);
            }
            exit(0); // Child process exits after handling
        }
    }
    close(new_socket);
    return 0;
}