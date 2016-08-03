#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
extern EFI_BOOT_SERVICES *BS;

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

void vfs_uefi_shutdown(vfs_fs_handler_t* this) {
}

int vfs_uefi_file_exists(vfs_fs_handler_t* this, char* path) {
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

     retval->fs_data = (char*)uefi_volume;

     retval->shutdown    = &vfs_uefi_shutdown;
     retval->file_exists = &vfs_uefi_file_exists;
     retval->open        = &vfs_uefi_open;
     retval->close       = &vfs_uefi_close;
     retval->read        = &vfs_uefi_read;
     retval->write       = &vfs_uefi_write;
     retval->lseek       = &vfs_uefi_lseek;
     retval->stat        = &vfs_uefi_stat;
     retval->fstat       = &vfs_uefi_fstat;
}
