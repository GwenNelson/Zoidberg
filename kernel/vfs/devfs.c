#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../k_vfs.h"
extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

#define IN_DEVFS
#include "devfs.h"

vfs_fs_type_t *devfs_fs_type = NULL;
char* devfs_fs_type_s = "devfs";

char** root_list = {
    "console",
    NULL
};

void vfs_devfs_shutdown(vfs_fs_handler_t* this) {
}

int vfs_devfs_file_exists(vfs_fs_handler_t* this, char* path) {
}

char** vfs_devfs_list_root_dir(vfs_fs_handler_t* this) {
       return root_list;
}

void* vfs_devfs_open(vfs_fs_handler_t* this, char* path, int flags) {
}

int vfs_devfs_close(vfs_fs_handler_t* this, void* fd) {
}

ssize_t vfs_devfs_read(vfs_fs_handler_t* this, void* fd, void* buf, size_t count) {
}

ssize_t vfs_devfs_write(vfs_fs_handler_t* this, void* fd, void* buf, size_t count) {
}

off_t vfs_devfs_lseek(vfs_fs_handler_t* this, void* fd, off_t offset, int whence) {
}

int vfs_devfs_stat(vfs_fs_handler_t* this, char* path, struct stat *buf) {
}

int vfs_devfs_fstat(vfs_fs_handler_t* this, void* fd, struct stat *buf) {
}



void vfs_devfs_setup(vfs_fs_handler_t* this, char* dev_name, char* mountpoint) {
     this->list_root_dir = &vfs_devfs_list_root_dir;
     this->shutdown      = &vfs_devfs_shutdown;
     this->file_exists   = &vfs_devfs_file_exists;
     this->list_root_dir = &vfs_devfs_list_root_dir;

     this->open          = &vfs_devfs_open;
     this->close         = &vfs_devfs_close;
     this->read          = &vfs_devfs_read;
     this->write         = &vfs_devfs_write;
     this->lseek         = &vfs_devfs_lseek;
     this->stat          = &vfs_devfs_stat;
     this->fstat         = &vfs_devfs_fstat;
}

void vfs_init_devfs_fs_type() {
     devfs_fs_type          = (vfs_fs_type_t*)calloc(sizeof(vfs_fs_type_t),1);
     devfs_fs_type->fs_type = devfs_fs_type_s;
     devfs_fs_type->setup   = &vfs_devfs_setup;
     klog("VFS",1,"devfs filesystem driver setup");
}
