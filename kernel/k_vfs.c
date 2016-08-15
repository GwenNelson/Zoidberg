#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"
#include <stdio.h>

#include "vfs/devuefi.h"
#include "vfs/uefi.h"
#include "vfs/devfs.h"

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

vfs_prefix_entry_t* locate_prefix(char* path) { // scan the prefix table for a file
   vfs_prefix_entry_t* p = vfs_prefix_list_first;
   int max_len=0;
   vfs_prefix_entry_t* retval = NULL;
   while(p != NULL) {
       if(strncmp(path,p->prefix_str,strlen(p->prefix_str))==0) {
          if(strlen(p->prefix_str) > max_len) {
             retval  = p;
             max_len = strlen(p->prefix_str);
          }
       }
       p = p->next;
   } 
   return retval;
}

vfs_fd_t* vfs_fopen(char* path, char* mode) {
   vfs_prefix_entry_t* p = locate_prefix(path);
   if(p==NULL) return NULL;
   if(p->fs_handler->file_exists(p->fs_handler,path)==0) return NULL;
   vfs_fd_t* retval = malloc(sizeof(vfs_fd_t));
   retval->fs_handler = p->fs_handler;
   retval->handler_fd = p->fs_handler->open(p->fs_handler,path, mode);
   return retval;
}

vfs_dir_fd_t* vfs_opendir(char* path) {
   vfs_dir_fd_t* retval = calloc(sizeof(vfs_dir_fd_t),1);
   if((path[0]=='/') && (strlen(path)==1)) {  // root is a special case - on readdir() it iterates through prefixes
      retval->is_root     = 1;
      retval->cur_prefix  = vfs_prefix_list_first;
      retval->prefix_dirs = NULL;
   } else {
      retval->is_root    = 0;
      retval->fs_handler = locate_prefix(path);
      if(retval->fs_handler == NULL) return NULL;
      retval->handler_fd = retval->fs_handler->opendir(retval->fs_handler,path);
   }
   return retval;
}

vfs_dirent_t* vfs_readdir(vfs_dir_fd_t* fd) {
   vfs_dirent_t* retval = calloc(sizeof(vfs_dirent_t),1);
   int i=0;
   if(fd->is_root==1) { // TODO - genericalise this, it's basically a special case for /, but the same algorithm should work for /dev and friends
     if((fd->cur_prefix->prefix_str[0]=='/') && (strlen(fd->cur_prefix->prefix_str)==1)) {
       if(fd->prefix_dirs == NULL) {
          fd->prefix_dirs = fd->cur_prefix->fs_handler->list_root_dir(fd->cur_prefix->fs_handler);
          fd->last_out    = -1;
       }
       fd->last_out++;
       if(fd->prefix_dirs[fd->last_out] == NULL) {
          free(fd->prefix_dirs);
          fd->prefix_dirs = NULL;
          fd->cur_prefix = fd->cur_prefix->next; // should fall straight through to if(fd->cur_prefix != NULL) below
       } else {
          strncpy(retval->d_name,fd->prefix_dirs[fd->last_out],255);
          return retval;
       }
     } 
     if(fd->cur_prefix != NULL) {
        for(i=0; (i<256) && (i<strlen(fd->cur_prefix->prefix_str+1)); i++) {
            retval->d_name[i]=0;
            if((fd->cur_prefix->prefix_str+1)[i] != '/') {
               retval->d_name[i]=(fd->cur_prefix->prefix_str+1)[i];
            }
        }
        fd->cur_prefix = fd->cur_prefix->next;
        return retval;
     } else {
        return NULL;
     }
   }
   return retval;
}


void vfs_init_types() {
     // TODO - find some way to make this more dynamic - EFI protocols perhaps?
     klog("VFS",1,"Setting up VFS filesystem types");
     vfs_init_devuefi_fs_type();
     vfs_add_type(devuefi_fs_type);

     vfs_init_uefi_fs_type();
     vfs_add_type(uefi_fs_type);
     
     vfs_init_devfs_fs_type();
     vfs_add_type(devfs_fs_type);
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

     vfs_simple_mount("devfs", "devfs", "/dev");

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
     vfs_dir_fd_t* root_dir_fd = vfs_opendir("/");
     vfs_dirent_t* dir_ent    = vfs_readdir(root_dir_fd);
     while(dir_ent != NULL) {
        klog("VFS",1,dir_ent->d_name);
        free(dir_ent);
        dir_ent    = vfs_readdir(root_dir_fd);
     }
}






