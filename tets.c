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

int getFileSize(FILE *file) {
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);
    return size;
}

int main() {
    FILE *file = fopen("files/image.jpg", "rb");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    int size_file = getFileSize(file);
    printf("Size is: %d\n", size_file);

    // Ensure buffer is large enough if you're using a fixed size
    char *buffer = malloc(size_file); // Dynamically allocate buffer based on file size
    if (buffer == NULL) {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    size_t bytesRead = fread(buffer, 1, size_file, file);
    if (bytesRead < size_file) {
        if (feof(file)) {
            printf("End of file reached.\n");
        } else if (ferror(file)) {
            perror("Error reading file");
            free(buffer);
            fclose(file);
            return 1;
        }
    }

    fclose(file);

    printf("buffer : %s\n", buffer);

    FILE* res = fopen("other.jpg", "wb");
    if (res == NULL) {
        perror("Failed to open output file");
        free(buffer);
        return 1;
    }

    size_t bytesWritten = fwrite(buffer, 1, bytesRead, res);
    if (bytesWritten < bytesRead) {
        perror("Error writing to file");
    } else {
        printf("Write operation successful.\n");
    }

    fclose(res);
    free(buffer); // Free the dynamically allocated memory

    return 0;
}
