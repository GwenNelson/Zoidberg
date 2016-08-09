#ifndef K_VFS_PROTO_H
#define K_VFS_PROTO_H

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>


typedef struct {
    UINTN              Signature;
    EFI_FILE_PROTOCOL FileProto;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL FileSystemProto;
    UINTN is_fs;  // set to 1 if this is for the file system protocol, 0 for the file protocol
    UINTN is_dir; // should be obvious...
    char* path;
} VFS_PROTO_PRIVATE_DATA;

#define VFS_PROTO_PRIVATE_DATA_SIGNATURE SIGNATURE_32 ( 'Z', 'V', 'F', 'S')

#define VFS_PROTO_PRIVATE_DATA_FROM_FILE(a) CR (a, VFS_PROTO_PRIVATE_DATA, FileProto, VFS_PROTO_PRIVATE_DATA_SIGNATURE)

#define VFS_PROTO_PRIVATE_DATA_FROM_FILESYS(a) CR (a, VFS_PROTO_PRIVATE_DATA, FileSystemProto, VFS_PROTO_PRIVATE_DATA_SIGNATURE)

void init_vfs_proto();

#endif

