#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <unistd.h>
#include "wfs.h"
#include <fcntl.h>

void *disk = NULL;
struct wfs_sb *superblock = NULL;
struct wfs_log_entry *inodenum_to_logentry(unsigned int ino)
{
    char *ptr = NULL;
    ptr = (char *)((char *)disk + sizeof(struct wfs_sb));
    struct wfs_log_entry *lep = (struct wfs_log_entry *)ptr;
    struct wfs_log_entry *final = NULL;
    // begin with first entry
    for (; ptr < (char *)disk + superblock->head; ptr += (sizeof(struct wfs_inode) + lep->inode.size))
    {
        lep = (struct wfs_log_entry *)ptr;
        if (lep->inode.inode_number == ino && lep->inode.deleted == 0)
        {
            final = lep;
        }
    }
    return final;
}

unsigned int find_max_inodenum()
{
    char *ptr = NULL;
    ptr = (char *)((char *)disk + sizeof(struct wfs_sb));
    struct wfs_log_entry *lep = (struct wfs_log_entry *)ptr;
    unsigned int inodenum = 0;
    // begin with first entry
    for (; ptr < (char *)disk + superblock->head; ptr += (sizeof(struct wfs_inode) + lep->inode.size))
    {
        lep = (struct wfs_log_entry *)ptr;
        if (lep->inode.inode_number > inodenum && lep->inode.deleted == 0)
        {
            inodenum = lep->inode.inode_number;
        }
    }
    return inodenum;
}

int main(int argc, char *argv[])
{
    char *path = argv[1];
    int fd = open(path, O_RDWR);
    if (fd < 0)
    {
        perror("Failed to open disk image");
        exit(1);
    }
    // get the file size
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("Failed to get file size");
        close(fd);
        exit(1);
    }
    // mmap disk
    disk = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED)
    {
        perror("Failed to map file");
        close(fd);
        exit(1);
    }
    superblock = (struct wfs_sb *)disk;
    char *ptr = (char *)((char *)disk + sizeof(struct wfs_sb));
    for (unsigned int i = 0; i <= find_max_inodenum(); i++)
    {
        struct wfs_log_entry *entry = inodenum_to_logentry(i);
        size_t entry_size = sizeof(struct wfs_inode) + entry->inode.size;
        memmove(ptr, entry, entry_size);
        ptr += entry_size;
    }
    memset(ptr, 0, (char *)disk + sb.st_size - ptr);
    // Update superblock->head to reflect the new end of the log
    superblock->head = ptr - (char *)disk;
    close(fd);
    return 0;
}
