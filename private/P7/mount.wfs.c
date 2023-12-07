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

char* extract_before_last_slash(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    // Find the last occurrence of '/'
    const char *last_slash = strrchr(str, '/');
    if (last_slash == NULL) {
        return NULL; // No slash found
    }

    // Calculate the length of the substring before the last slash
    size_t length = last_slash - str;

    // Allocate memory for the new substring
    char *result = malloc(length + 1); // +1 for the null terminator
    if (result == NULL) {
        return NULL; // Memory allocation failed
    }

    // Copy the substring into the new string
    strncpy(result, str, length);
    result[length] = '\0'; // Null-terminate the string

    return result;
}

struct wfs_log_entry *inodenum_to_logentry(unsigned int ino){
    // printf("number = %d\n", ino);
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
    printf("=========================\n");
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

    // find root first
    logptr = inodenum_to_logentry(inodenum);

    puts(path);

    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        // printf("token = %s    \n", token);

        // logptr = inodenum_to_logentry(inodenum);

        // printf("inode number is %d\n",logptr->inode.inode_number);
        // printf("inode mode is %d\n",logptr->inode.mode);
        // printf("inode size is %d\n",logptr->inode.size);
        // struct wfs_dentry* data = (struct wfs_dentry*)logptr->data;

        // for (struct wfs_dentry* p = data;p < data + logptr->inode.size; p+= DENTRY_SIZE){
        //     printf("inode first dir dentry = %s\n", p->name);
        //     printf("inode first dir dentry inodenumber = %lu\n", p->inode_number);

        //     printf("p = %ld\n",(unsigned long)p);
        //     printf("logptr->data + logptr->inode.size = %ld\n",(unsigned long)(logptr->data + logptr->inode.size));
        // }

        if (logptr == NULL){
            return NULL;
        }
        printf("inode of dir = %lu\n",inodenum);
        inodenum = name_to_inodenum(token, logptr);
        if(inodenum == -1) {
            return NULL;
        }

        logptr = inodenum_to_logentry(inodenum);


        token = strtok(NULL, "/");
    }
    free(path_copy); // 释放复制的字符串
    // printf("logentry of  %d\n",logptr->inode.inode_number);
    return logptr;

}

struct wfs_log_entry *path_to_parent_logentry(const char *path){
    char* parent_path = extract_before_last_slash(path);
    struct wfs_log_entry *logptr = path_to_logentry(parent_path);
    if(logptr == NULL){
        return NULL;
    }
    free(parent_path);
    return logptr;
}



static int wfs_getattr(const char *path, struct stat *stbuf) {
    memset(stbuf, 0, sizeof(struct stat));

    struct wfs_log_entry *logptr = path_to_logentry(path);
    if (logptr == NULL) {
        return -ENOENT;  // No such file or directory
    }

    // Assuming logptr->mode contains both file type and permissions
    stbuf->st_mode = logptr->inode.mode;

    // Set the number of hard links. Typically, this is 1 for files
    stbuf->st_nlink = logptr->inode.links;

    // Set the file size
    stbuf->st_size = logptr->inode.size;

    // Set the owner's user ID and group ID
    stbuf->st_uid = logptr->inode.uid;
    stbuf->st_gid = logptr->inode.gid;

    // Set timestamps
    stbuf->st_mtime = logptr->inode.mtime; // Modification time

    return 0; // Success
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
    // Check if 'path' is a directory
    // ...
    // Iterate over each entry in the directory
    // should fill filler with . and ..
    struct wfs_log_entry *l = path_to_logentry(path);
    if(l == NULL){
        return -ENOENT;
    }

    // "."
    struct stat st1;
    memset(&st1, 0, sizeof(st1));
    st1.st_ino = l->inode.inode_number; // inode number for the entry
    st1.st_mode = l->inode.mode;      // DT_REG for files, DT_DIR for directories, etc.

    if (filler(buf, ".", &st1, 0)) {
    }


    char *end_ptr = (char*)l + (sizeof(struct wfs_inode) + l->inode.size);
    char *cur = l->data; // begin at dentry
    struct wfs_dentry *d_ptr = (struct wfs_dentry*) cur;
    for( ; cur < end_ptr; cur += DENTRY_SIZE ){
        d_ptr = (struct wfs_dentry*) cur;
        if (inodenum_to_logentry(d_ptr->inode_number)->inode.deleted == 0){
            // not deleted
            struct stat st;
            memset(&st, 0, sizeof(st));
            st.st_ino = d_ptr->inode_number; // inode number for the entry
            st.st_mode = inodenum_to_logentry(d_ptr->inode_number)->inode.mode;        // DT_REG for files, DT_DIR for directories, etc.

            // Use the filler function to add the entry to the buffer
            if (filler(buf, d_ptr->name, &st, 0)) {
            }
        }
    }

    return 0; // Success
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
