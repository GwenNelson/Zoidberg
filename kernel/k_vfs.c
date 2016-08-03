#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
extern EFI_BOOT_SERVICES *BS;

vfs_prefix_entry_t *vfs_prefix_list_first = NULL;
vfs_prefix_entry_t *vfs_prefix_list_last  = NULL;

char boot_path[32];

void vfs_init() {
//     klog("VFS",1,"VFS mapping /dev/uefi to UEFI volumes");
//     vfs_fs_handler_t* dev_uefi_handler;
//     dev_uefi_handler = get_vfs_handler_dev_uefi();
//     vfs_mount(dev_uefi_handler,"uefi","/dev/uefi");

     klog("VFS",1,"VFS mapping / to UEFI initrd");
     vfs_fs_handler_t* initrd_handler;
     initrd_handler = get_vfs_handler_uefi("initrd");
     vfs_mount(initrd_handler, "/dev/uefi/initrd", "/");

     char* boot_volume = "fs0"; // TODO - make this actually check
     snprintf(boot_path,32,"/dev/uefi/%s",boot_volume);

     klog("VFS",1,"VFS mapping /boot to UEFI %s",boot_volume);
     vfs_fs_handler_t* boot_handler;
     boot_handler = get_vfs_handler_uefi(boot_volume);
     vfs_mount(boot_handler, boot_path, "/boot/");


}

void vfs_mount(vfs_fs_handler_t* fs_handler, char* dev_name, char* mountpoint) {
     vfs_prefix_entry_t* new_entry;
     new_entry = (vfs_prefix_entry_t*)malloc(sizeof(vfs_prefix_entry_t));
     BS->SetMem((void*)new_entry,sizeof(vfs_prefix_entry_t),0);

     new_entry->prefix_str = mountpoint;
     new_entry->dev_name   = dev_name;
     new_entry->fs_handler = fs_handler;
     new_entry->next       = NULL;

     // TODO - add lock here and anywhere else we fiddle with the prefix list
     if(vfs_prefix_list_first==NULL) {
        new_entry->prev       = NULL;
        vfs_prefix_list_first = new_entry;
        vfs_prefix_list_last  = new_entry;
     } else {
        vfs_prefix_list_last->next = new_entry;
        new_entry->prev            = vfs_prefix_list_last;
        vfs_prefix_list_last = new_entry;
     }

}

void dump_vfs() {
     klog("VFS",1,"Dumping VFS");
     vfs_prefix_entry_t* p = vfs_prefix_list_first;
     while(p != NULL) {
        klog("VFS",1,"%s on %s type %s",p->dev_name,p->prefix_str,p->fs_handler->fs_type);
        if(p->fs_handler->list_root_dir != NULL) {
           char** root_files = p->fs_handler->list_root_dir(p->fs_handler);
           char** f=root_files;
           while(*f != NULL) {
              if(*f != NULL) {
                 klog("VFS",1,"\t%s%s",p->prefix_str,*f);
                 f++;
              }
           }
           
        }
        p = p->next;
        if(p==NULL) return;
     }
}


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

char* vfs_uefi_type = "uefi";

vfs_fs_handler_t* get_vfs_handler_uefi(char* uefi_volume) {
     vfs_fs_handler_t* retval;
     retval = (vfs_fs_handler_t*)malloc(sizeof(vfs_fs_handler_t));
     BS->SetMem((void*)retval,sizeof(vfs_fs_handler_t),0);

     retval->fs_data = (void*)uefi_volume;
     retval->fs_type = vfs_uefi_type;

     retval->shutdown      = &vfs_uefi_shutdown;
     retval->file_exists   = &vfs_uefi_file_exists;
     retval->list_root_dir = &vfs_uefi_list_root_dir;

     retval->open          = &vfs_uefi_open;
     retval->close         = &vfs_uefi_close;
     retval->read          = &vfs_uefi_read;
     retval->write         = &vfs_uefi_write;
     retval->lseek         = &vfs_uefi_lseek;
     retval->stat          = &vfs_uefi_stat;
     retval->fstat         = &vfs_uefi_fstat;
     return retval;
}


char* vfs_uefi_dev_type = "devuefi";

vfs_fs_handler_t *get_vfs_handler_dev_uefi() {
     vfs_fs_handler_t* retval;
     retval = (vfs_fs_handler_t*)malloc(sizeof(vfs_fs_handler_t));
     BS->SetMem((void*)retval,sizeof(vfs_fs_handler_t),0);
    
     // TODO - actual implementation here
     //        just relay to the UEFI shell and treat volumes as block devices
     //        mount can cheat by using the "uefi" filesystem type to use the UEFI handler
     //        or can (later) mount them directly as proper filesystems
 
     retval->fs_type = vfs_uefi_dev_type;
     return retval;
}
