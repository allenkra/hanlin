#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include "wfs.h"

/**
 * Open the disk file (or device).
Initialize the filesystem structures.
Write these structures to the disk.
This process is akin to formatting a disk with a new filesystem.
*/



void initialize_fs(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("Failed to open disk image");
        exit(1);
    }

    // get the file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Failed to get file size");
        close(fd);
        return 1;
    }

    // mmap disk
    void *mapped = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        return 1;
    }

    // init sb and log entry
    struct wfs_sb *sbPtr = (struct wfs_sb *)mapped;
    // set the value of  Superblock
    // TODO
    
    struct wfs_log_entry *logPtr = (struct wfs_log_entry *)((char *)mapped + sizeof( struct wfs_sb));  // log entry is located just after Superblock
    // set the value of Log Entry
    // TODO

    // synchorize to disk
    if (msync(mapped, sb.st_size, MS_SYNC) == -1) {
        perror("Failed to sync changes");
    }

    if (munmap(mapped, sb.st_size) == -1) {
        perror("Failed to unmap memory");
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <disk_path>\n", argv[0]);
        return 1;
    }

    // pass the disk path
    initialize_fs(argv[1]);

    return 0;
}
