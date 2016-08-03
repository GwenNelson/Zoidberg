#ifndef K_VFS_H
#define K_VFS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

typedef struct vfs_fs_handler_t vfs_fs_handler_t;
typedef struct vfs_fs_handler_t {
     // used by the file handler to represent the underlying device
     void* fs_data;

     // what is shown by the mount command etc
     char* fs_type;

     // called when unmounting
     void     (*shutdown)(vfs_fs_handler_t* this);

     // needed by the VFS layer
     int      (*file_exists)(vfs_fs_handler_t* this, char* path);   // simple boolean check
     char**   (*list_root_dir)(vfs_fs_handler_t* this);             // returns an array of strings, caller must free()

     // standard I/O operations
     void*    (*open)(vfs_fs_handler_t* this, char* path, int flags);
     int      (*close)(vfs_fs_handler_t* this, void* fd);
     ssize_t  (*read)(vfs_fs_handler_t* this, void* fd, void* buf, size_t count);
     ssize_t  (*write)(vfs_fs_handler_t* this, void* fd, void* buf, size_t count);
     off_t    (*lseek)(vfs_fs_handler_t* this, void* fd, off_t offset, int whence);
     int      (*stat)(vfs_fs_handler_t* this, char* path, struct stat *buf);
     int      (*fstat)(vfs_fs_handler_t* this, void* fd, struct stat *buf);
} vfs_fs_handler_t;

typedef struct vfs_prefix_entry_t vfs_prefix_entry_t;

struct vfs_prefix_entry_t {
     char* prefix_str;
     char* dev_name;
     vfs_fs_handler_t *fs_handler;
     vfs_prefix_entry_t* next;
     vfs_prefix_entry_t* prev;
};

// returns a vfs_fs_handler_t representing all the UEFI volumes (for use by /dev/uefi)
vfs_fs_handler_t *get_vfs_handler_dev_uefi();

// returns a vfs_fs_handler_t representing a UEFI volume, no trailing :
vfs_fs_handler_t *get_vfs_handler_uefi(char* uefi_volume);

void vfs_init();
void vfs_simple_mount(char* fs_type, char* dev_name, char* mountpoint);
void vfs_mount(vfs_fs_handler_t* fs_handler, char* dev_name, char* mountpoint);

// dump the mount table etc
void dump_vfs();

// either param can be NULL, but one must be non-null
//  whichever is the most recent matching prefix entry will be removed
void vfs_umount(char* dev_name, char* mountpoint);

#endif
