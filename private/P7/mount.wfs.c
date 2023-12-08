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

const char* path_to_file_name(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    // Find the last occurrence of '/'
    const char *last_slash = strrchr(str, '/');
    if (last_slash == NULL) {
        return NULL; // No slash found
    }

    // Return the substring after the last slash
    return last_slash + 1;
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

unsigned int find_max_inodenum(){
    // printf("number = %d\n", ino);
    char *ptr = NULL;
    ptr = (char*)((char *)disk + sizeof( struct wfs_sb));
    struct wfs_log_entry *lep = (struct wfs_log_entry *) ptr;
    unsigned int inodenum = 0;
    // begin with first entry
    for(;ptr < (char*)disk + superblock->head; ptr += (sizeof(struct wfs_inode) + lep->inode.size)){
        lep = (struct wfs_log_entry *) ptr;
        if (lep->inode.inode_number > inodenum && lep->inode.deleted == 0){
            inodenum = lep->inode.inode_number;
        }
    }
    // not found
    printf("max inodenum = %d\n",inodenum);
    return inodenum;
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

int delete_dentry(unsigned int inodenum, struct wfs_log_entry *l){
    char *end_ptr = (char*)l + (sizeof(struct wfs_inode) + l->inode.size);
    char *cur = l->data; // begin at dentry
    struct wfs_dentry *d_ptr;

    for( ; cur < end_ptr; cur += DENTRY_SIZE ){
        d_ptr = (struct wfs_dentry*) cur;

        if(d_ptr->inode_number == inodenum){
            // Calculate the size of memory to move
            size_t move_size = end_ptr - (cur + DENTRY_SIZE);
            
            // Move subsequent dentries forward
            if(move_size > 0){
                memmove(cur, cur + DENTRY_SIZE, move_size);
            }
            // Return success
            return 0;
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
    if(strlen(parent_path) == 0){
        printf("path_to_parent_logentry = null %s\n", parent_path);
        return inodenum_to_logentry(0);
    }
    struct wfs_log_entry *logptr = path_to_logentry(parent_path);
    if(logptr == NULL){

        return NULL;
    }
    printf("path_to_parent_logentry %s\n", parent_path);
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
    const char* file_name= path_to_file_name(path);
    struct wfs_log_entry *parent_log = path_to_parent_logentry(path);
    unsigned int new_inodenum = find_max_inodenum() + 1;
    printf("filename is %s\n", file_name);
    if(name_to_inodenum((char*)file_name, parent_log) != -1){
        // already exist
        return -EEXIST;
    }
    // append new parent log
    struct wfs_log_entry *new_parent_log = (struct wfs_log_entry *)((char*)disk + superblock->head);

    memcpy((char*)new_parent_log, (char*)parent_log, sizeof(struct wfs_inode) + parent_log->inode.size);

    struct wfs_dentry *new_dentry = (void*)((char*)new_parent_log->data + new_parent_log->inode.size);
    strcpy(new_dentry->name, file_name);
    new_dentry->inode_number = new_inodenum; 

    // update size and head
    new_parent_log->inode.size = new_parent_log->inode.size + DENTRY_SIZE;
    superblock->head = superblock->head + (uint32_t)(sizeof(struct wfs_inode) + new_parent_log->inode.size);

    // append new log for new inode 
    struct wfs_log_entry *new_file_entry = (struct wfs_log_entry *)((char*)disk + superblock->head);
    new_file_entry->inode.inode_number = new_inodenum;
    new_file_entry->inode.atime = time(NULL);
    new_file_entry->inode.mtime = time(NULL);
    new_file_entry->inode.ctime = time(NULL);
    new_file_entry->inode.deleted = 0;
    new_file_entry->inode.flags = 0;
    new_file_entry->inode.gid = getuid();
    new_file_entry->inode.uid = getuid();
    new_file_entry->inode.links = 1;
    new_file_entry->inode.mode = mode | S_IFREG;
    new_file_entry->inode.size = 0;

    superblock->head = superblock->head + (uint32_t)(sizeof(struct wfs_inode));

    return 0;
}

static int wfs_mkdir(const char *path, mode_t mode) {
    // Implementation of mkdir function to create a directory
    // ...
    // create directory
    // append two
    const char* file_name= path_to_file_name(path);
    struct wfs_log_entry *parent_log = path_to_parent_logentry(path);
    unsigned int new_inodenum = find_max_inodenum() + 1;

    if(name_to_inodenum((char*)file_name, parent_log) != -1){
        // already exist
        printf("2e20e20e2j0j\n");
        return -EEXIST;
    }
    // append new parent log
    struct wfs_log_entry *new_parent_log = (struct wfs_log_entry *)((char*)disk + superblock->head);
    memcpy((char*)new_parent_log, (char*)parent_log, sizeof(struct wfs_inode) + parent_log->inode.size);
    // *new_parent_log = *parent_log;

    struct wfs_dentry *new_dentry = (void*)((char*)new_parent_log->data + new_parent_log->inode.size);
    strcpy(new_dentry->name, file_name);
    new_dentry->inode_number = new_inodenum; 

    // update size and head
    new_parent_log->inode.size = new_parent_log->inode.size + DENTRY_SIZE;
    superblock->head = superblock->head + (uint32_t)(sizeof(struct wfs_inode) + new_parent_log->inode.size);

    // append new log for new inode 
    struct wfs_log_entry *new_file_entry = (struct wfs_log_entry *)((char*)disk + superblock->head);
    new_file_entry->inode.inode_number = new_inodenum;
    new_file_entry->inode.atime = time(NULL);
    new_file_entry->inode.mtime = time(NULL);
    new_file_entry->inode.ctime = time(NULL);
    new_file_entry->inode.deleted = 0;
    new_file_entry->inode.flags = 0;
    new_file_entry->inode.gid = getuid();
    new_file_entry->inode.uid = getuid();
    new_file_entry->inode.links = 1;
    new_file_entry->inode.mode = S_IFDIR | mode;
    new_file_entry->inode.size = 0;

    superblock->head = superblock->head + (uint32_t)(sizeof(struct wfs_inode));

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
    if (file_entry == NULL) {
        // File not found or error
        return -ENOENT;
    }

    len = file_entry->inode.size;


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
    if(offset < 0){
        // bad write
        return 0;
    }
    struct wfs_log_entry* old_log = path_to_logentry(path);
    if(old_log == NULL){
        return -ENOENT;
    }

    int new_size = old_log->inode.size;

    if(offset + size > old_log->inode.size){
        // update size
        new_size = offset + size;
    }
    struct wfs_log_entry *new_log = (struct wfs_log_entry*)((char*)disk + superblock->head);
    memcpy((char*)new_log, (char*)old_log, sizeof(struct wfs_inode) + old_log->inode.size);
    new_log->inode.size = new_size;
    memcpy((char*)new_log->data + offset, (char*)buf, size);
    new_log->inode.mtime = time(NULL);

    superblock->head = superblock->head + (uint32_t)(sizeof(struct wfs_inode) ) + new_log->inode.size;
    return size;
}

static int wfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    // Check if 'path' is a directory
    // ...
    // Iterate over each entry in the directory
    // should fill filler with . and ..
    struct wfs_log_entry *l = path_to_logentry(path);
    struct wfs_log_entry *parent = path_to_parent_logentry(path);
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

    struct stat st2;
    memset(&st2, 0, sizeof(st2));
    st2.st_ino = parent->inode.inode_number;
    st2.st_mode = l->inode.mode;
    if (filler(buf, "..", &st2, 0)) {
    }

    char *end_ptr = (char*)l + (sizeof(struct wfs_inode) + l->inode.size);
    char *cur = l->data; // begin at dentry
    struct wfs_dentry *d_ptr = (struct wfs_dentry*) cur;
    for( ; cur < end_ptr; cur += DENTRY_SIZE ){
        d_ptr = (struct wfs_dentry*) cur;
        if (inodenum_to_logentry(d_ptr->inode_number) != NULL){
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
    struct wfs_log_entry *log_ptr = path_to_logentry(path);
    if(log_ptr == NULL){
        return -ENOENT;
    }
    struct wfs_log_entry *parent_log = path_to_parent_logentry(path);
    if(parent_log == NULL){
        return -ENONET;
    }
    log_ptr->inode.deleted = 1;

    struct wfs_log_entry *new_log = (struct wfs_log_entry*)((char*)disk + superblock->head);

    memcpy((char*)new_log, (char*)parent_log, sizeof(struct wfs_inode) + parent_log->inode.size);

    if(delete_dentry(log_ptr->inode.inode_number, new_log) == -1){
        return -ENONET;
    }

    new_log->inode.size = parent_log->inode.size - DENTRY_SIZE;
    new_log->inode.mtime = time(NULL);

    superblock->head = superblock->head + (uint32_t)(sizeof(struct wfs_inode) ) + new_log->inode.size;

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
