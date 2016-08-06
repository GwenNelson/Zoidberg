#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"

#include "vfs/devuefi.h"
//include "vfs/uefi.h"
//include "vfs/devfs.h"

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EfiShell.h>

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;


vfs_prefix_entry_t *vfs_prefix_list_first = NULL;
vfs_prefix_entry_t *vfs_prefix_list_last  = NULL;

vfs_fs_type_t* vfs_fs_type_list_first = NULL;
vfs_fs_type_t* vfs_fs_type_list_last  = NULL;

char boot_path[32];
char* vfs_uefi_type = "uefi"; // mounts VFS volumes

void vfs_init_types() {
     // TODO - find some way to make this more dynamic - EFI protocols perhaps?
     klog("VFS",1,"Setting up VFS filesystem types");
     vfs_init_devuefi_fs_type();
     vfs_add_type(devuefi_fs_type);
}

void vfs_add_type(vfs_fs_type_t *fs_type) {
     // TODO - locking (as in vfs_mount)
     // TODO - use macros to handle these linked lists, abstract it out to something more generic too so it's possible to switch containers to hashmap etc
     if(vfs_fs_type_list_first==NULL) {
        fs_type->prev          = NULL;
        vfs_fs_type_list_first = fs_type;
        vfs_fs_type_list_last  = fs_type;
     } else {
        vfs_fs_type_list_last->next = fs_type;
        fs_type->prev               = vfs_fs_type_list_last;
        vfs_fs_type_list_last       = fs_type;

     }
}

void vfs_init() {
     vfs_init_types();

     vfs_simple_mount("devuefi","uefi","/dev/uefi/");

//     vfs_simple_mount("uefi","/dev/uefi/initrd","/");

//     char* boot_volume = "fs0"; // TODO - make this actually check
//     snprintf(boot_path,32,"/dev/uefi/%s",boot_volume);
//     vfs_simple_mount("uefi",boot_path,"/boot/");

}

void vfs_simple_mount(char* fs_type, char* dev_name, char* mountpoint) {
     // TODO - some sort of dynamic table of filesystem types
     //        perhaps simply a list of vfs_fs_handler_t structs and add a new setup() method to the struct
     //        then simply check the fs_type field of each until a match is found, and then call setup()
     // TODO - if fs_type is NULL, try and autodetect the filesystem

     vfs_fs_handler_t *fs_handler = NULL;
     vfs_fs_type_t    *fs_t_iter  = NULL;
     fs_t_iter                    = vfs_fs_type_list_first;
     while(fs_t_iter != NULL) {
       printf("%#llx\n",fs_t_iter);
       if(fs_t_iter->fs_type != NULL) {
          if(strncmp(fs_t_iter->fs_type,fs_type,strlen(fs_type)) == 0) {
             fs_handler = (vfs_fs_handler_t*)calloc(1,sizeof(vfs_fs_handler_t));
             strncpy(fs_handler->fs_type,fs_type,MAX_VFS_TYPE_LEN);
             fs_handler->setup = fs_t_iter->setup;
             fs_handler->setup(fs_handler,dev_name,mountpoint);
          }
          fs_t_iter = fs_t_iter->next;
       }
     }
     // TODO - error checking
     if(fs_handler != NULL) vfs_mount(fs_handler,dev_name,mountpoint);
     
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



vfs_fs_handler_t* get_vfs_handler_uefi(char* uefi_volume) {
     vfs_fs_handler_t* retval;
     retval = (vfs_fs_handler_t*)malloc(sizeof(vfs_fs_handler_t));
     BS->SetMem((void*)retval,sizeof(vfs_fs_handler_t),0);

     retval->fs_data = calloc(sizeof(char),strlen(uefi_volume)+1);
     strncpy((char*)(retval->fs_data),uefi_volume,strlen(uefi_volume));
     strncpy(retval->fs_type,vfs_uefi_type,MAX_VFS_TYPE_LEN);

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




