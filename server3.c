#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define PORT 8086
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

    return 0; // Success
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
    char *encoded_data;

    // Open the file
    file = fopen(path, "rb");
    if (file == NULL)
    {
        send(client_fd, "24", 3, 0);
        send(client_fd, "404 FILE NOT FOUND\r\n\r\n", 24, 0);
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
        send(client_fd, "25", 3, 0);
        send(client_fd, "500 INTERNAL ERROR\r\n\r\n", 25, 0);
        return -1; // Indicate failure
    }

    // Read the file into the buffer
    fread(buffer,1,file_size,file);
    buffer[file_size] = '\0'; // Null-terminate the buffer to treat it as a C-string
    fclose(file);             // Close the file as soon as we're done with it

    // Encode the content to Base64
    if (Base64Encode((const char *)buffer, &encoded_data) != 0)
    {
        free(buffer); // Make sure to free the buffer if encoding fails
        send(client_fd, "25", 3, 0);
        send(client_fd, "500 INTERNAL ERROR\r\n\r\n", 25, 0);
        return -1; // Indicate failure
    }
    int res_size = file_size + 14;
    char res_sizeS[1024];
    memset(res_sizeS, 0, res_size);

    sprintf(res_sizeS, "%d", res_size);
    // Free the buffer as it's no longer needed
    free(buffer);
    send(client_fd, res_sizeS, 1024, 0);
    char response[res_size];
    memset(response, 0, res_size);
    strcat(response, "200 OK\r\n");
    strcat(response, encoded_data);
    strcat(response, "\r\n\r\n");
    // Send the response with the encoded data
    send(client_fd, response, res_size, 0);

    // Free the encoded data after sending it
    free(encoded_data);

    return 0; // Indicate success
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

    // Ignore SIGCHLD to prevent zombie processes
    signal(SIGCHLD, SIG_IGN);
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
            bytes_recv = recv(new_socket, c, sizeof(c), 0); // recving the choice
            if (bytes_recv < 0)
            {
                perror("recv failed");
                close(new_socket);
                close(sock);
                exit(errno);
            }

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

            printf("the path is: %s\n ", full_path);
            if (strcmp(c, "1"))
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
            else if (!strcmp(c, "0"))
            {
                // lockFile(full_path);
                // if (POST() < 0)
                // {
                //     perror("POST failed");
                //     close(new_socket);
                //     close(sock);
                //     exit(errno);
                // }
                // unlockFile(full_path);
            }

            close(new_socket);
            exit(0); // Child process exits after handling
        }
        else
        {
            // Parent process
            close(new_socket); // Parent doesn't need this
        }
    }
    close(sock);
}