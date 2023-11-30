#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * Open the disk file (or device).
Initialize the filesystem structures.
Write these structures to the disk.
This process is akin to formatting a disk with a new filesystem.
*/


// Define the size of the file system and other constants
#define FS_SIZE 10485760 // Example size, e.g., 10MB

// Function to initialize the file system
void initialize_fs(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("Failed to open disk image");
        exit(1);
    }

    // Allocate and zero-initialize a buffer
    char *buffer = calloc(1, FS_SIZE);
    if (!buffer) {
        perror("Failed to allocate memory");
        close(fd);
        exit(1);
    }

    // Initialize file system structures here
    // ...

    // Write the buffer to the file
    if (write(fd, buffer, FS_SIZE) != FS_SIZE) {
        perror("Failed to write file system to disk image");
        free(buffer);
        close(fd);
        exit(1);
    }

    free(buffer);
    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk_path>\n", argv[0]);
        return 1;
    }

    initialize_fs(argv[1]);

    return 0;
}
