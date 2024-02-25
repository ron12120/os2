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
#include <math.h>
#include <poll.h>

#define BUFFER_SIZE 1024
#define PORT 80
#define ADDR "127.0.0.1"

int isListFile(const char *filename)
{
    // Find the last occurrence of '.' in the filename
    const char *dot = strrchr(filename, '.');

    // Check if a dot is found and it's not the first character in the filename
    if (dot != NULL && dot != filename)
    {
        // Compare the file extension (case-sensitive) with ".list"
        if (strcmp(dot + 1, "list") == 0)
        {
            return 1; // File has the ".list" extension
        }
    }

    return 0; // File does not have the ".list" extension
}
int countLines(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("fopen failed");
        return -1;
    }
    int count = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF)
    {
        if (ch == '\n')
            count++;
    }
    fclose(file);
    return count;
}
char *getLineFromFile(const char *filename, int lineNumber)
{
    FILE *file = fopen(filename, "r");
    char buffer[1024]; // Adjust the buffer size as needed
    int currentLine = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        currentLine++;

        if (currentLine == lineNumber)
        {
            // Close the file before returning the result
            fclose(file);
            // Remove newline character at the end of the line
            char *newlinePos = strchr(buffer, '\n');
            if (newlinePos != NULL)
            {
                *newlinePos = '\0';
            }
            // Allocate memory for the line and copy the content
            char *result = strdup(buffer);
            if (result == NULL)
            {
                perror("Error allocating memory");
            }
            return result;
        }
    }

    fclose(file);

    // Line number is out of bounds
    printf("Error: Line number %d not found in file.\n", lineNumber);
    return NULL;
}

int getFileSize(FILE *file)
{
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    return size;
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
    BIO_write(bio, message, messageLen);        // Write the data to be encoded
    BIO_flush(bio);                             // Flush the BIO to ensure all data is written
    BIO_free_all(bio);                          // Free the BIO chain
    fclose(stream);                             // Close the memory buffer
    return 0;                                   // Success
}

char *getBase64Content(const char *input)
{
    const char *delimiter = "\r\n";
    char *token;
    char *result = NULL;
    int lineCount = 0;

    // Create a mutable copy of the input for strtok
    char *inputCopy = strdup(input);
    if (!inputCopy)
        return NULL; // Check for allocation failure

    // Tokenize the input string with strtok
    token = strtok(inputCopy, delimiter);
    while (token != NULL)
    {
        lineCount++;
        if (lineCount == 2)
        {                           // Assuming the BASE64 content is always on the second line
            result = strdup(token); // Duplicate the token to return
            break;
        }
        token = strtok(NULL, delimiter);
    }
    free(inputCopy); // Free the allocated memory for the copy
    return result;   // Remember, the caller is responsible for freeing this
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

void send_post_request(int socket, char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        perror("Error opening file for reading");
        return;
    }
    int size_file = getFileSize(file);
    char *buffer = (char *)malloc(size_file);
    if (buffer == NULL)
    {
        perror("malloc error");
        fclose(file);
        close(socket);
        exit(EXIT_FAILURE);
    }
    memset(buffer, 0, size_file);
    fread(buffer, 1, size_file, file);
    fclose(file);
    char *encoded_data;
    Base64Encode(buffer, size_file, &encoded_data);
    int response_size = strlen("POST\r\n") + strlen(file_path) + strlen("\r\n") + strlen(encoded_data) + strlen("\r\n\r\n");
    char response_size_str[BUFFER_SIZE];
    char response[response_size];
    strcpy(response, "POST\r\n");
    strcat(response, file_path);
    strcat(response, "\r\n");
    strcat(response, encoded_data);
    strcat(response, "\r\n\r\n");
    sprintf(response_size_str, "%d", response_size);
    send(socket, response_size_str, BUFFER_SIZE, 0);
    sleep(0.5);
    send(socket, response, response_size, 0);
    free(buffer);
}

int openSock()
{
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
    return sock;
}

int main(int argc, char **argv)
{

    if (argc != 3 || (strcmp(argv[1], "GET") != 0 && strcmp(argv[1], "POST") != 0))
    {
        fprintf(stderr, "Usage: %s <GET/POST> <remote_path>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (isListFile(argv[2]) == 1)
    {
        int num_of_files = countLines(argv[2]);
        struct pollfd fds[num_of_files];
        for (int i = 0; i < num_of_files; i++)
        {
            sleep(0.5);
            fds[i].fd = openSock();
            fds[i].events = POLLIN;
        }

        for (int i = 0; i < num_of_files; i++)
        {
            sleep(0.5);
            send_get_request(fds[i].fd, getLineFromFile(argv[2], i + 1));
        }

        if (poll(fds, num_of_files, -1) < 0)
        {
            perror("poll failed");
            return -1;
        }

        for (int i = 0; i < num_of_files; i++)
        {
            char size[BUFFER_SIZE];
            sleep(0.5);
            recv(fds[i].fd, size, BUFFER_SIZE, 0);
            int size_int = atoi(size);
            sleep(0.5);
            char response[size_int];
            memset(response, 0, size_int);
            recv(fds[i].fd, response, size_int, 0);
            char code[4];
            strncpy(code, response, 3);
            code[3] = '\0';
            if (strcmp(code, "200") == 0)
            {
                printf("200 ok\r\n\r\n");
                char *base64con = getBase64Content(response);
                FILE *file = fopen(getLineFromFile(argv[2], i + 1), "wb");
                if (file == NULL)
                {
                    perror("fopen failed");
                    close(fds[i].fd);
                    exit(errno);
                }

                unsigned char *buffer;
                size_t decodedLength = 0;
                Base64Decode(base64con, &buffer, &decodedLength);
                fwrite(buffer, 1, decodedLength, file);
                fclose(file);
            }
            else if (strcmp(code, "404") == 0)
            {
                printf("404 FILE NOT FOUND\r\n\r\n");
            }
            else
            {
                printf("500 INTERNAL ERROR\r\n\r\n");
            }
        }
    }
    else if (strcmp(argv[1], "GET") == 0)
    {
        int sock = openSock();
        send_get_request(sock, argv[2]);
        char size[BUFFER_SIZE];
        sleep(0.5);
        recv(sock, size, BUFFER_SIZE, 0);
        int size_int = atoi(size);
        sleep(0.5);
        char response[size_int];
        memset(response, 0, size_int);
        recv(sock, response, size_int, 0);
        char code[4];
        strncpy(code, response, 3);
        code[3] = '\0';
        if (strcmp(code, "200") == 0)
        {
            printf("200 ok\r\n\r\n");
            char *base64con = getBase64Content(response);
            FILE *file = fopen(argv[2], "wb");
            if (file == NULL)
            {
                perror("fopen failed");
                close(sock);
                exit(errno);
            }

            unsigned char *buffer;
            size_t decodedLength = 0;
            Base64Decode(base64con, &buffer, &decodedLength);
            fwrite(buffer, 1, decodedLength, file);
            fclose(file);
            close(sock);
        }
        else if (strcmp(code, "404") == 0)
        {
            printf("404 FILE NOT FOUND\r\n\r\n");
            close(sock);
        }
        else
        {
            printf("500 INTERNAL ERROR\r\n\r\n");
            close(sock);
        }
    }
    else if (strcmp(argv[1], "POST") == 0)
    {
        int sock = openSock();
        send_post_request(sock, argv[2]);
        printf("File sent successfully.\n");
        close(sock);
    }

    return 0;
}