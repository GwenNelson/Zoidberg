#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"

vfs_prefix_entry_t *vfs_prefix_list_first = NULL;
vfs_prefix_entry_t *vfs_prefix_list_last  = NULL;

void vfs_init() {
     char* boot_volume = "fs0"; // TODO - make this actually check
     char boot_path[32];
     snprintf(boot_path,32,"/dev/uefi/%s",boot_volume);

     klog("VFS",1,"VFS mapping /boot to UEFI %s",boot_volume);
     vfs_fs_handler_t* boot_handler;
     boot_handler = get_vfs_handler_uefi(boot_volume);
     vfs_mount(boot_handler, boot_path, "/boot");

     klog("VFS",1,"VFS mapping / to UEFI initrd:");
     vfs_fs_handler_t* initrd_handler;
     initrd_handler = get_vfs_handler_uefi("initrd");
     vfs_mount(initrd_handler, "/dev/uefi/initrd", "/");
}

