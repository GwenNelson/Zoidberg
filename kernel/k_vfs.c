#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"
#include <stdio.h>

#include "vfs/devuefi.h"
#include "vfs/uefi.h"
//include "vfs/devfs.h"

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EfiShell.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>


extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;


vfs_prefix_entry_t *vfs_prefix_list_first = NULL;
vfs_prefix_entry_t *vfs_prefix_list_last  = NULL;

vfs_fs_type_t* vfs_fs_type_list_first = NULL;
vfs_fs_type_t* vfs_fs_type_list_last  = NULL;

char boot_path[PATH_MAX];
char* vfs_uefi_type = "uefi"; // mounts VFS volumes

char* vfs_uefi_root = "initrd:"; // TODO - make this ZOIDBERG and implement the SIMPLE_FILE_SYSTEM protocol to map the VFS for EFI functions

FILE* vfs_fopen(char* path, char* mode) {
      // for now, this only works with initrd
      char uefi_path[PATH_MAX];
      snprintf(uefi_path,"%s\\path",PATH_MAX);
      char* s = uefi_path;
      while(*s != 0) {
         if(*s == '/') {
             *s = '\\';
         }
      }
      return fopen(uefi_path,mode);
}





void vfs_init_types() {
     // TODO - find some way to make this more dynamic - EFI protocols perhaps?
     klog("VFS",1,"Setting up VFS filesystem types");
     vfs_init_devuefi_fs_type();
     vfs_add_type(devuefi_fs_type);

     vfs_init_uefi_fs_type();
     vfs_add_type(uefi_fs_type);
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

extern char* argv0; // import from k_main
void vfs_init() {
     vfs_init_types();

     vfs_simple_mount("devuefi","uefi","/dev/uefi/");

     vfs_simple_mount("uefi","/dev/uefi/initrd","/");

     // this is of course a terrible and messy hack
     char kernel_path[PATH_MAX];
     strncpy(kernel_path,argv0,PATH_MAX);
     snprintf(boot_path,PATH_MAX,"/dev/uefi/%s",strtok(kernel_path,":"));
     vfs_simple_mount("uefi",boot_path,"/boot/");

     init_vfs_proto();
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






