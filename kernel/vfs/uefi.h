#ifndef VFS_UEFI_H
#define VFS_UEFI_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../k_vfs.h"

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

#ifndef IN_VFSUEFI
extern vfs_fs_type_t *uefi_fs_type;
#endif

void vfs_init_uefi_fs_type();

#endif
