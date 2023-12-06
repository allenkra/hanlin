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

void *disk = NULL;
struct wfs_sb *superblock = NULL;

int count_slashes(const char *str) {
    int count = 0;
    while (*str) { 
        if (*str == '/') {
            count++; 
        }
        str++; 
    }
    return count;
}

struct wfs_log_entry *inodenum_to_logentry(unsigned int ino){
    char *ptr = NULL;
    ptr = (char*)((char *)disk + sizeof( struct wfs_sb));
    struct wfs_log_entry *lep = (struct wfs_log_entry *) ptr;
    struct wfs_log_entry *final = NULL;
    // begin with first entry
    for(;ptr < (char*)disk + superblock->head; ptr += (sizeof(struct wfs_inode) + lep->inode.size)){
        lep = (struct wfs_log_entry *) ptr;
        if (lep->inode.inode_number == ino && lep->inode.deleted == 0){
            final = lep;
        }
    }
    // not found
    return final;
}


unsigned long name_to_inodenum(char *name, struct wfs_log_entry *l){
    char *end_ptr = (char*)l + (sizeof(struct wfs_inode) + l->inode.size);
    char *cur = l->data; // begin at dentry
    struct wfs_dentry *d_ptr = (struct wfs_dentry*) cur;
    for( ; cur < end_ptr; cur += DENTRY_SIZE ){
        d_ptr = (struct wfs_dentry*) cur;
        if (strcmp(name, d_ptr->name) == 0){
            return d_ptr->inode_number;
        }
    }
    // not found
    return -1;
}


struct wfs_log_entry *path_to_logentry(const char *path){

    char *path_copy = strdup(path);
    if (path_copy == NULL) {
        perror("Failed to duplicate path string");
        return NULL;
    }
    // int slashes = count_slashes(path);

    unsigned long inodenum = 0;

    struct wfs_log_entry *logptr = NULL;

    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        logptr = inodenum_to_logentry(inodenum);
        if (logptr == NULL){
            return NULL;
        }
        
        if(logptr->inode.mode == S_IFDIR){
            // directory
            inodenum = name_to_inodenum(token, logptr);
        }
        else{
            // file
            // nothing to do
        }
        token = strtok(NULL, "/");
    }
    logptr = inodenum_to_logentry(inodenum);
    free(path_copy); // 释放复制的字符串
    return logptr;

}

static int wfs_getattr(const char *path, struct stat *stbuf) {
    // Implementation of getattr function to retrieve file attributes
    // ...
    // ok
    return 0; // Return 0 on success
}

static int wfs_mknod(const char *path, mode_t mode, dev_t dev) {
    // Implementation of mknod function to create file node
    // ...
    // create file
    // append two new log, one for dir one for itself 
    return 0;
}

static int wfs_mkdir(const char *path, mode_t mode) {
    // Implementation of mkdir function to create a directory
    // ...
    // create directory
    // append two
    return 0;
}

static int wfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    printf("Requested file: %s\n", path);

    // Example: Let's assume 'path' points to a file in your custom filesystem.
    // You would need to find this file within your filesystem's structure.
    // This example assumes you have a function `my_fs_get_file_content`
    // that retrieves the file's content based on the file's path.
    
    int len;
    int ret = 0;
    struct wfs_log_entry *file_entry = path_to_logentry(path);

    len = file_entry->inode.size;

    if (file_entry == NULL) {
        // File not found or error
        return -ENOENT;
    }

    if (offset < len) {
        if (offset + size > len) {
            size = len - offset;
        }
        memcpy(buf, file_entry->data + offset, size);
        ret = size;
    } else {
        ret = 0; // Offset is past the end of the file, overflow
    }

    return ret;
}


static int wfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    // Implementation of write function to write data to a file
    // ...
    // append one new logentry of itself
    return 0;
}

static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    // Implementation of readdir function to read a directory
    // ...
    // ok
    return 0;
}

static int wfs_unlink(const char *path) {
    // Implementation of unlink function to remove a file
    // ...
    // ok
    // set links and deleted
    // append new entry of parent dir
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

    // should be like ./mount.wfs -f -s disk mnt or without -f
    int i;
    char *disk_arg = NULL;
    // Filter argc and argv here and then pass it to fuse_main

    // Iterate over arguments
    for (i = 1; i < argc; i++) {
        if (!(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "-s") == 0)) {
            disk_arg = argv[i]; // Save the disk argument
            // Remove 'disk' from argv
            memmove(&argv[i], &argv[i + 1], (argc - i - 1) * sizeof(char*));
            argc--; 
            i--; 
            break;
        }
    }

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
    disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Failed to map file");
        close(fd);
        exit(1);
    }

    superblock = (struct wfs_sb *)disk;

    // for (i = 0; i < argc; i++) {
    //     printf("argv[%d] = %s\n", i, argv[i]);
    // }

    int fuse_return_value = fuse_main(argc, argv, &ops, NULL);

    if (munmap(disk, sb.st_size) == -1) {
        perror("Failed to unmap memory");
    }

    close(fd);

    return fuse_return_value;
}
