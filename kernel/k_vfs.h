#ifndef K_VFS_H
#define K_VFS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EfiShell.h>

#define MAX_VFS_TYPE_LEN 32

typedef struct vfs_fs_handler_t vfs_fs_handler_t;
typedef struct vfs_fs_handler_t {
     // used by the file handler to represent the underlying device
     void* fs_data;

     // what is shown by the mount command etc
     char fs_type[MAX_VFS_TYPE_LEN];

     // setup the FS handler - mountpoint param can probably be NULL most of the time, but should be provided where possible
     // in case a driver requires the mountpoint for some reason
     void     (*setup)(vfs_fs_handler_t* this, char* dev_name, char* mountpoint);

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

typedef struct vfs_fs_type_t vfs_fs_type_t;
typedef struct vfs_fs_type_t {
     // TODO - add some sort of probe callback to this struct so we can probe devices for supported filesystem drivers
     char* fs_type;
     vfs_fs_type_t *next;
     vfs_fs_type_t *prev;
     void (*setup)(vfs_fs_handler_t* this, char* dev_name, char* mountpoint);  // a pointer to the setup() method for vfs_fs_handler_t struct
} vfs_fs_type_t;

void vfs_init_types();  // init the builtin types, should only be called by vfs_init()
void vfs_add_type(vfs_fs_type_t *fs_type); // install a filesystem type after the driver is loaded and ready to rock - should eventually be able to dynamically load drivers from ELF
void vfs_init();        // init the VFS layer and mount the mandatory filesystems the system needs in order to operate
void vfs_simple_mount(char* fs_type, char* dev_name, char* mountpoint); // mount a filesystem, duh - calls vfs_mount() to implement
void vfs_mount(vfs_fs_handler_t* fs_handler, char* dev_name, char* mountpoint); // mount a filesystem, but you have to lookup the handler and init the struct first

// dump the mount table etc to system console
void dump_vfs();

// either param can be NULL, but one must be non-null
//  whichever is the most recent matching prefix entry will be removed
void vfs_umount(char* dev_name, char* mountpoint);

// simple wrapper so EDK2 libc can still be used to do stdio
FILE* vfs_fopen(char* path, char* mode);

#endif
