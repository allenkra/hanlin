#include <stddef.h>
#include <stdint.h>

#ifndef MOUNT_WFS_H_
#define MOUNT_WFS_H_

#define MAX_FILE_NAME_LEN 32
#define DENTRY_SIZE 40
#define WFS_MAGIC 0xdeadbeef

struct wfs_sb {
    uint32_t magic;
    uint32_t head;
};

struct wfs_inode {
    unsigned int inode_number;
    unsigned int deleted;       // 1 if deleted, 0 otherwise
    unsigned int mode;          // type. S_IFDIR if the inode represents a directory or S_IFREG if it's for a file
    unsigned int uid;           // user id
    unsigned int gid;           // group id
    unsigned int flags;         // flags
    unsigned int size;          // size in bytes
    unsigned int atime;         // last access time
    unsigned int mtime;         // last modify time
    unsigned int ctime;         // inode change time (the last time any field of inode is modified)
    unsigned int links;         // number of hard links to this file (this can always be set to 1)
};

struct wfs_dentry {
    char name[MAX_FILE_NAME_LEN];
    unsigned long inode_number;
};

struct wfs_log_entry {
    struct wfs_inode inode;
    char data[]; //should be dentry array when it is a directory
};

struct wfs_log_entry *inodenum_to_logentry(unsigned int ino);
unsigned long name_to_inodenum(char *name, struct wfs_log_entry *l);
struct wfs_log_entry *path_to_logentry(const char *path);




#endif
