#ifndef VFS_DEVFS_H
#define VFS_DEVFS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../k_vfs.h"

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

#ifndef IN_DEVFS
extern vfs_fs_type_t *devfs_fs_type;
#endif

void vfs_init_devfs_fs_type();

#endif
