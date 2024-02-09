#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>

#define PORT 8080
#define BACKLOG 10 // How many pending connections queue will hold

struct flock lock;

void lock_file(char *file_path)
{
    int fd = open(file_path, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct flock lock;
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;    // Exclusive lock
    lock.l_whence = SEEK_SET; // Beginning of file
    lock.l_start = 0;         // Offset from l_whence
    lock.l_len = 0;           // 0 means lock the whole file

    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("fcntl");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void unlock_file(char *file_path)
{
    int fd = open(file_path, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    lock.l_type = F_UNLCK;
    if (fcntl(fd, F_SETLK, &lock) == -1)
    {
        perror("fcntl unlock");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

int main()
{

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[BACKLOG + 1];
    memset(fds, 0, sizeof(fds)); // Initialize pollfd structure
    fds[0].fd = 0;
    fds[0].events = POLLIN;

    if (listen(server_fd, BACKLOG + 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        printf("Awaiting connections...\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        close(new_socket);
    }
    return 0;
}
