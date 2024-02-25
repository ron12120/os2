#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <math.h>

#define BUFFER_SIZE 1024
#define PORT 80
#define ADDR "127.0.0.1"

int lockFile(FILE *file, int lockType) {
    int fd = fileno(file);
    struct flock fl;

    // Initialize the lock structure
    fl.l_type = lockType;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    // Try to acquire/release the lock
    if (fcntl(fd, lockType == F_UNLCK ? F_SETLK : F_SETLKW, &fl) == -1) {
        perror(lockType == F_UNLCK ? "Failed to unlock file" : "Failed to lock file");
        return -1;
    }

    printf("File %s successfully\n", lockType == F_UNLCK ? "unlocked" : "locked");

    return 0;
}

int getFileSize(FILE *file)
{
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    return size;
}

int calcDecodeLength(const char *b64input)
{
    int len = strlen(b64input);
    int padding = 0;

    if (b64input[len - 1] == '=' && b64input[len - 2] == '=') // last two chars are =
        padding = 2;
    else if (b64input[len - 1] == '=') // last char is =
        padding = 1;

    return (int)len * 0.75 - padding;
}

int Base64Decode(const char *b64message, unsigned char **buffer, size_t *outLen)
{
    BIO *bio, *b64;
    int decodeLen = calcDecodeLength(b64message);

    *buffer = (unsigned char *)malloc(decodeLen);
    if (*buffer == NULL)
    {
        return 1; // Memory allocation failure
    }
    FILE *stream = fmemopen((void *)b64message, strlen(b64message), "r");
    if (!stream)
    {
        free(*buffer);
        return 2; // File stream open failure
    }

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer

    int len = BIO_read(bio, *buffer, strlen(b64message));
    if (len != decodeLen)
    {
        free(*buffer);
        BIO_free_all(bio);
        fclose(stream);
        return 3; // Decode length mismatch or read failure
    }

    *outLen = len; // Set the output length for the caller

    BIO_free_all(bio);
    fclose(stream);

    return 0; // Success
}

int Base64Encode(char *message, size_t messageLen, char **buffer)
{
    BIO *bio, *b64;
    FILE *stream;
    int encodedSize = 4 * ceil((double)messageLen / 3); // Calculate the size needed for the encoded data
    *buffer = (char *)malloc(encodedSize + 1);          // Allocate memory for the encoded string

    stream = fmemopen(*buffer, encodedSize + 1, "w"); // Open a memory buffer for output
    if (stream == NULL)
    {
        free(*buffer);
        return -1; // Return an error if the memory buffer couldn't be opened
    }

    b64 = BIO_new(BIO_f_base64());              // Create a new BIO for base64
    bio = BIO_new_fp(stream, BIO_NOCLOSE);      // Create a new BIO that writes to the memory buffer
    bio = BIO_push(b64, bio);                   // Chain the base64 BIO onto the buffer BIO
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // Don't use newlines to flush buffer

    BIO_write(bio, message, messageLen); // Write the data to be encoded
    BIO_flush(bio);                      // Flush the BIO to ensure all data is written
    BIO_free_all(bio);                   // Free the BIO chain
    fclose(stream);                      // Close the memory buffer

    return 0; // Success
}

void handle_post_request(int client_socket, char* remote_path, char *server_path, char *base64)
{ 
    unsigned char *buffer;
    size_t size = 0;
    Base64Decode(base64, &buffer, &size);
    int path_size=strlen(server_path)+strlen(remote_path);
    char fullpath[path_size];
    memset(fullpath,0,path_size);
    strcpy(fullpath, server_path);
    strcat(fullpath, remote_path);
    FILE *file = fopen(fullpath, "wb");
    if (file == NULL)
    {
        perror("fopen failed");
        close(client_socket);
        exit(errno);
    }
    lockFile(file, F_WRLCK);
    fwrite(buffer, 1, size, file);
    lockFile(file, F_UNLCK);
    fclose(file);
    close(client_socket);
}

void handle_get_request(int client_socket, char *remote_path, char *server_path)
{
    char full_path[BUFFER_SIZE];
    sprintf(full_path, "%s%s", server_path, remote_path);

    printf("Attempting to open file: %s\n", full_path);

    FILE *file = fopen(full_path, "rb");

    if (file == NULL)
    {
        // File not found, send 404 response
        printf("File not found: %s\n", full_path);
        char *size_msg = "23";
        send(client_socket, size_msg, BUFFER_SIZE, 0);
        sleep(0.5);
        send(client_socket, "404 FILE NOT FOUND\r\n\r\n", 23, 0);
    }

    int size_file = getFileSize(file);
    char *buffer = (char *)malloc(size_file);
    if (buffer == NULL)
    {
        perror("malloc error");
        fclose(file);
        char *size_msg = "23";
        send(client_socket, size_msg, BUFFER_SIZE, 0);
        sleep(0.5);
        send(client_socket, "500 INTERNAL ERROR\r\n\r\n", 23, 0);
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, size_file);
    fread(buffer, 1, size_file, file);
    fclose(file);
    char size_res[BUFFER_SIZE];
    char *encoded_data;
    Base64Encode(buffer, size_file, &encoded_data);
    int response_size = strlen("200 OK\r\n") + strlen(encoded_data) + strlen("\r\n\r\n");
    char response[response_size];
    strcpy(response, "200 OK\r\n");
    strcat(response, encoded_data);
    strcat(response, "\r\n\r\n");
    sprintf(size_res, "%d", response_size);
    sleep(0.5);
    send(client_socket, size_res, BUFFER_SIZE, 0);
    sleep(0.5);
    send(client_socket, response, response_size, 0);
    close(client_socket);
    free(buffer);
}

void handle_client(int client_socket, char *server_path)
{
    char size[BUFFER_SIZE];
    ssize_t bytesRead = recv(client_socket, size, BUFFER_SIZE, 0);
    if (bytesRead < 0)
    {
        perror("recv size failed");
        close(client_socket);
    }
    int size_res = atoi(size) + 1;
    char buffer[size_res];
    bytesRead = recv(client_socket, buffer, size_res, 0);
    if (bytesRead < 0)
    {
        perror("recv request failed");
        close(client_socket);
    }
    buffer[bytesRead] = '\0'; // Null-terminate the received data

    // Parse the request type, remote path, and contents (if applicable)
    char request_type[5];
    char remote_path[BUFFER_SIZE];

    sscanf(buffer, "%s %s", request_type, remote_path);
   

    if (strcmp(request_type, "GET") == 0)
    {
        handle_get_request(client_socket, remote_path, server_path);
    }
    else if (strcmp(request_type, "POST") == 0)
    {
        int size_base64= size_res-10-strlen(remote_path);
        char base64[size_base64];
        sscanf(buffer, "%s %s\r\n%s", request_type, remote_path,base64);
        handle_post_request(client_socket, remote_path, server_path,base64);
    }
    else
    {
        // Invalid request type, send 500 response
        send(client_socket, "500 INTERNAL ERROR\r\n", strlen("500 INTERNAL ERROR\r\n"), 0);
        close(client_socket);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // Get the server's working directory
    char server_path[BUFFER_SIZE];
    strncpy(server_path, argv[1], BUFFER_SIZE);
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

    if (listen(sock, 100) < 0)
    {
        perror("listen failed");
        close(sock);
        exit(errno);
    }
    printf("listening on: %s:%d\n", ADDR, PORT);

    // Accept connections and handle requests
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
        if (pid == 0)
        {
            // Child process
            close(sock); // Close server socket in child process
            handle_client(new_socket, server_path);
            exit(EXIT_SUCCESS);
        }
    }

    return 0;
}
