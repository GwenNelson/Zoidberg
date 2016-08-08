#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../k_vfs.h"
extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

#define IN_VFSUEFI
#include "devuefi.h"

vfs_fs_type_t *uefi_fs_type = NULL;
char* uefi_fs_type_s = "uefi";

void vfs_uefi_shutdown(vfs_fs_handler_t* this) {
}

int vfs_uefi_file_exists(vfs_fs_handler_t* this, char* path) {
}

char** vfs_uefi_list_root_dir(vfs_fs_handler_t* this) {
    char root_path[32];
    snprintf(root_path,32,"%s:/",(char*)(this->fs_data));

    char** retval = (char**)calloc(sizeof(char*),1024);

    int i=0;
    DIR* dir_fd = opendir(root_path);
    struct dirent *dir_ent=NULL;
    if(dir_fd==NULL) {
       klog("VFS",0,"Error opening UEFI root on %s",root_path);
       free(retval);
       return NULL;
    }
    
    while(dir_ent = readdir(dir_fd)) {
       if(dir_ent==NULL) {
          retval[i] = NULL;
       } else {

          retval[i] = (char*)calloc(1,128);
          wcstombs(retval[i],dir_ent->d_name,128);
          i++;
       }
    }
    closedir(dir_fd);
    return retval;
}

void* vfs_uefi_open(vfs_fs_handler_t* this, char* path, int flags) {
}

int vfs_uefi_close(vfs_fs_handler_t* this, void* fd) {
}

ssize_t vfs_uefi_read(vfs_fs_handler_t* this, void* fd, void* buf, size_t count) {
}

ssize_t vfs_uefi_write(vfs_fs_handler_t* this, void* fd, void* buf, size_t count) {
}

off_t vfs_uefi_lseek(vfs_fs_handler_t* this, void* fd, off_t offset, int whence) {
}

int vfs_uefi_stat(vfs_fs_handler_t* this, char* path, struct stat *buf) {
}

int vfs_uefi_fstat(vfs_fs_handler_t* this, void* fd, struct stat *buf) {
}



void vfs_uefi_setup(vfs_fs_handler_t* this, char* dev_name, char* mountpoint) {
     char* uefi_volume   = strrchr(dev_name,'/')+1;
     this->fs_data       = calloc(sizeof(char),strlen(uefi_volume)+1);
     strncpy((char*)(this->fs_data),uefi_volume,strlen(uefi_volume)+1);

     this->list_root_dir = &vfs_uefi_list_root_dir;
     this->shutdown      = &vfs_uefi_shutdown;
     this->file_exists   = &vfs_uefi_file_exists;
     this->list_root_dir = &vfs_uefi_list_root_dir;

     this->open          = &vfs_uefi_open;
     this->close         = &vfs_uefi_close;
     this->read          = &vfs_uefi_read;
     this->write         = &vfs_uefi_write;
     this->lseek         = &vfs_uefi_lseek;
     this->stat          = &vfs_uefi_stat;
     this->fstat         = &vfs_uefi_fstat;
}

void vfs_init_uefi_fs_type() {
     uefi_fs_type          = (vfs_fs_type_t*)calloc(sizeof(vfs_fs_type_t),1);
     uefi_fs_type->fs_type = uefi_fs_type_s;
     uefi_fs_type->setup   = &vfs_uefi_setup;
     klog("VFS",1,"uefi filesystem driver setup");
}
