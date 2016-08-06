#ifndef DEVUEFI_H
#define DEVUEFI_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../k_vfs.h"

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

#ifndef IN_DEVUEFI
extern vfs_fs_type_t *devuefi_fs_type;
#endif

void vfs_init_devuefi_fs_type();

#endif
