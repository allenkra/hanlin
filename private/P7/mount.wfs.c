#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "wfs.h"

static int wfs_getattr(const char *path, struct stat *stbuf) {
    // Implementation of getattr function to retrieve file attributes
    // ...
    return 0; // Return 0 on success
}

static int wfs_mknod(const char *path, mode_t mode, dev_t dev) {
    // Implementation of mknod function to create file node
    // ...
    return 0;
}

static int wfs_mkdir(const char *path, mode_t mode) {
    // Implementation of mkdir function to create a directory
    // ...
    return 0;
}

static int wfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Implementation of read function to read data from a file
    // ...
    return 0;
}

static int wfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Implementation of write function to write data to a file
    // ...
    return 0;
}

static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    // Implementation of readdir function to read a directory
    // ...
    return 0;
}

static int wfs_unlink(const char *path) {
    // Implementation of unlink function to remove a file
    // ...
    return 0;
}

static struct fuse_operations ops = {
    .getattr    = wfs_getattr,
    .mknod      = wfs_mknod,
    .mkdir      = wfs_mkdir,
    .read       = wfs_read,
    .write      = wfs_write,
    .readdir    = wfs_readdir,
    .unlink     = wfs_unlink,
};

int main(int argc, char *argv[]) {

    // should be like ./mount.wfs -f -s disk mnt
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <disk_path>\n", argv[0]);
        return 1;
    }
    // Filter argc and argv here and then pass it to fuse_main
    int i = 3;
    char *disk_arg = NULL; // Pointer to store disk argument

    disk_arg = argv[i];
    // Remove 'disk' from argv
    memmove(&argv[i], &argv[i + 1], (argc - i - 1) * sizeof(char*));
    argc--;

    // open disk
    int fd = open(disk_arg, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("Failed to open disk image");
        exit(1);
    }

    // get the file size
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("Failed to get file size");
        close(fd);
        exit(1);
    }

    // mmap disk
    void *disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        exit(1);
    }

    int fuse_return_value = fuse_main(argc, argv, &ops, NULL);

    if (munmap(disk, sb.st_size) == -1) {
        perror("Failed to unmap memory");
    }

    close(fd);

    return fuse_return_value;
}
