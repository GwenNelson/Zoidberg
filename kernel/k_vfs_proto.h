#ifndef K_VFS_PROTO_H
#define K_VFS_PROTO_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "k_vfs.h"


EFI_FILE_PROTOCOL ZoidbergVFSFileInterface;

typedef struct {
    UINTN             Signature;
    EFI_FILE_PROTOCOL FileProto;
    UINTN vfs_root; // set to 1 for /
    UINTN is_dir; // should be obvious...
    UINTN dir_pos;  // when reading directories, current position
    char* path;
    vfs_fs_handler_t* fs_handler;
    void* fd;     // the fd for use by the filesystem handler
} VFS_PROTO_PRIVATE_DATA;

#define VFS_PROTO_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ( 'Z', 'V', 'F', 'S')

#define VFS_DATA_FROM_THIS(a) \
    CR(a, VFS_PROTO_PRIVATE_DATA, FileProto, VFS_PROTO_PRIVATE_DATA_SIGNATURE)


void init_vfs_proto();

#endif

