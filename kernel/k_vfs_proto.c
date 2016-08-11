#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"
#include <stdio.h>

#include "k_vfs_proto.h"

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
extern EFI_RUNTIME_SERVICES *RT;
extern EFI_HANDLE gImageHandle;

// TODO - setup private context struct for the EFI_FILE_PROTOCOL struct
// TODO - import ext3 driver
// TODO - implement a tar driver so initrd can be a tarball
// TODO - abstraction layer for getting an EFI_FILE_PROTOCOL directly from /dev/uefi/whatever
// TODO - perhaps a gzip layer over block devices?

EFI_STATUS EFIAPI ZoidbergVFSOpen(
 IN EFI_FILE_PROTOCOL *This,
 OUT EFI_FILE_PROTOCOL **NewHandle,
 IN CHAR16 *FileName,
 IN UINT64 OpenMode,
 IN UINT64 Attributes
 ) {
 VFS_PROTO_PRIVATE_DATA *Private;
// char filename_path[PATH_MAX];
// wcstombs(filename_path,FileName,PATH_MAX);
 char* filename_path = ".";
 int i=0;
 for(i=0; i < strlen(filename_path); i++) {
    if (filename_path[i]=='\\') {
        filename_path[i] = '/';
    }
 }
 Private = VFS_DATA_FROM_THIS(This);
 klog("VFS",1,"Open %s", filename_path);

 if(Private->is_dir == 1) {
   if(strncmp(filename_path,".",1)==0) { // just return This for attempts to open .
      klog("VFS",1,"Returning . directory");
      VFS_PROTO_PRIVATE_DATA *new_handle = calloc(sizeof(VFS_PROTO_PRIVATE_DATA),1);
      new_handle->vfs_root = Private->vfs_root;
      new_handle->is_dir   = 1;
      new_handle->dir_pos  = 0;
      new_handle->path = NULL;
      new_handle->FileProto = ZoidbergVFSFileInterface;
      *NewHandle = &(new_handle->FileProto);
      klog("VFS",1,"Allocated new file proto for . directory");
      return EFI_SUCCESS;
   } else {
      klog("VFS",1,"Returning NOT_FOUND");
      return EFI_NOT_FOUND; // placeholder for now
   }
 } else {
   klog("VFS",1,"Returning unsupported!");
   return EFI_UNSUPPORTED;
 }


 klog("VFS",1,"Attempted open %s on file_proto",filename_path);
 return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI ZoidbergVFSClose(
 IN EFI_FILE_PROTOCOL *This) {
 klog("VFS",1,"Freeing struct");
 VFS_PROTO_PRIVATE_DATA *Private;
 Private = VFS_DATA_FROM_THIS(This);
 free(Private->path);
 free(Private);
 return EFI_SUCCESS;
}

EFI_STATUS EFIAPI ZoidbergVFSRead(
 IN EFI_FILE_PROTOCOL *This,
 IN OUT UINTN *BufferSize,
 OUT VOID *Buffer) {
 VFS_PROTO_PRIVATE_DATA *Private;
 Private = VFS_DATA_FROM_THIS(This); 
 if(Private->is_dir == 1) {
    if(Private->dir_pos > 0) {
       *BufferSize = 0;
       
       return EFI_SUCCESS;
    } else {
       if(BufferSize < sizeof(EFI_FILE_INFO)+4) {
          *BufferSize = sizeof(EFI_FILE_INFO) + 4;
          return EFI_BUFFER_TOO_SMALL;
       }
       EFI_FILE_INFO *file_info    = (EFI_FILE_INFO*)Buffer;
       file_info->Size             = *BufferSize;
       file_info->FileSize         = 0;
       file_info->PhysicalSize     = 0;

       RT->GetTime(&(file_info->CreateTime),NULL);
       RT->GetTime(&(file_info->LastAccessTime),NULL);
       RT->GetTime(&(file_info->ModificationTime),NULL);
       file_info->Attribute = EFI_FILE_DIRECTORY;
       mbstowcs(file_info->FileName,".",2);
       Private->dir_pos = 0;
       return EFI_SUCCESS;
    }
 }

 klog("VFS",1,"Attempted read on file_proto");
 *BufferSize = 0;
 return EFI_SUCCESS;
}

EFI_STATUS EFIAPI ZoidbergVFSGetInfo(
IN EFI_FILE_PROTOCOL *This,
 IN EFI_GUID *InformationType,
 IN OUT UINTN *BufferSize,
 OUT VOID *Buffer
 ) {

 UINTN RequiredSize;
 if(CompareGuid(InformationType, &gEfiFileSystemInfoGuid)) {
   RequiredSize = sizeof(EFI_FILE_SYSTEM_INFO);
   if(*BufferSize < RequiredSize) {
      *BufferSize = RequiredSize;
      return EFI_BUFFER_TOO_SMALL;
   }
   Buffer = calloc(sizeof(EFI_FILE_SYSTEM_INFO),1);
   EFI_FILE_SYSTEM_INFO *fs_info = (EFI_FILE_SYSTEM_INFO*)Buffer;
   fs_info->Size       = sizeof(EFI_FILE_SYSTEM_INFO);
   fs_info->ReadOnly   = FALSE;
   fs_info->VolumeSize = 1024; // dummy
   fs_info->FreeSpace  = 1099511627776; // silly big number just so we don't error out on writes
   fs_info->BlockSize  = 512;
   return EFI_SUCCESS;
 } if(CompareGuid(InformationType, &gEfiFileInfoGuid)) { 
   RequiredSize = sizeof(EFI_FILE_INFO)+2;
   if(*BufferSize < RequiredSize) {
      *BufferSize = RequiredSize;
      return EFI_BUFFER_TOO_SMALL;
   }
   Buffer = calloc(sizeof(EFI_FILE_INFO)+2,1);
   EFI_FILE_INFO *file_info = (EFI_FILE_INFO*)Buffer;
   file_info->Size = sizeof(EFI_FILE_INFO)+2;
   file_info->FileSize = 0;
   file_info->PhysicalSize = 0;
   file_info->Attribute = EFI_FILE_DIRECTORY;
   mbstowcs(file_info->FileName,".",2);
   return EFI_SUCCESS;
 } else {
   return EFI_UNSUPPORTED;
 }
}

EFI_FILE_PROTOCOL ZoidbergVFSFileInterface = {
  .Revision    = EFI_FILE_PROTOCOL_REVISION,
  .Open        = ZoidbergVFSOpen,
  .Close       = ZoidbergVFSClose,
//  .Delete      = ZoidbergVFSDelete,
  .Read        = ZoidbergVFSRead,
/*  .Write       = ZoidbergVFSWrite,
  .GetPosition = ZoidbergVFSGetPosition,
  .SetPosition = ZoidbergVFSSetPosition,*/
  .GetInfo     = ZoidbergVFSGetInfo,
/*  .SetInfo     = ZoidbergVFSSetInfo,
  .Flush       = ZoidbergVFSFlush,
  .OpenEx      = ZoidbergVFSOpenEx,
  .ReadEx      = ZoidbergVFSReadEx,
  .WriteEx     = ZoidbergVFSWriteEx,
  .FlushEx     = ZoidbergVFSFlushEx*/

};

typedef struct __attribute__((__packed__)) sZOIDBERG_VFS_DEVICE_PATH
{
        EFI_DEVICE_PATH Header;
        EFI_GUID        Guid;
        EFI_DEVICE_PATH EndDevicePath;
} ZOIDBERG_VFS_DEVICE_PATH;

ZOIDBERG_VFS_DEVICE_PATH vfs_devpath_proto =
{
        MESSAGING_DEVICE_PATH,
        MSG_VENDOR_DP,
        sizeof(ZOIDBERG_VFS_DEVICE_PATH) - sizeof(EFI_DEVICE_PATH),
        0,
        // {d533a70d-7e54-4302-b6d7-bbe2c5672a1}
          0x6533a70d, 0x7e54, 0x4302, 0xb6, 0xd7, 0xbb, 0xe2, 0xc5, 0x67, 0x2c, 0xa1,
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        sizeof(EFI_DEVICE_PATH)
};



EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *new_vfs_proto;

EFI_STATUS
EFIAPI
ZoidbergVFSOpenVolume(
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL                **File
  ) {
     VFS_PROTO_PRIVATE_DATA *private = calloc(sizeof(VFS_PROTO_PRIVATE_DATA),1);
     if(private == NULL) return EFI_OUT_OF_RESOURCES;
     klog("VFS",1,"Allocated private struct for zoidberg VFS protocol at %#llx",private);
     private->Signature = VFS_PROTO_PRIVATE_DATA_SIGNATURE;
     private->vfs_root   = 1;
     private->fs_handler = NULL;
     private->dir_pos = 0;
     private->is_dir = 1;
     private->path = malloc(2);
     snprintf(private->path,2,"/");
     private->FileProto = ZoidbergVFSFileInterface;
     *File = &(private->FileProto);
   return EFI_SUCCESS; 
}

EFI_HANDLE vfs_handle;
void init_vfs_proto() {
     klog("VFS",1,"Installing VFS protocol");
     new_vfs_proto = calloc(sizeof(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL),1);
     new_vfs_proto->Revision   = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
     new_vfs_proto->OpenVolume = ZoidbergVFSOpenVolume;

     EFI_STATUS s = BS->InstallProtocolInterface(&vfs_handle,
                                                 &gEfiDevicePathProtocolGuid,
                                                 EFI_NATIVE_INTERFACE,
                                                 &vfs_devpath_proto);

     s = BS->InstallProtocolInterface(&vfs_handle,&gEfiSimpleFileSystemProtocolGuid,EFI_NATIVE_INTERFACE,new_vfs_proto);
     if(s==EFI_SUCCESS) {
        klog("VFS",1,"Added protocol interface to handle %#llx", vfs_handle);
     } else {
        klog("VFS",0,"Could not install protocol!");
     }

     EFI_SHELL_PROTOCOL *shell_proto;

     s = BS->OpenProtocol(
            gImageHandle,
            &gEfiShellProtocolGuid,
            &shell_proto,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
     if (EFI_ERROR(s)) {
        s = BS->LocateProtocol(
                &gEfiShellProtocolGuid,
                NULL,
                &shell_proto
                );
      }



     UINTN HandleCount;
     EFI_HANDLE *HandleBuffer;
     UINTN HandleIndex;
     s = BS->LocateHandleBuffer(AllHandles,NULL,NULL,&HandleCount,&HandleBuffer);
     for(HandleIndex=0; HandleIndex < HandleCount; HandleIndex++) {
         BS->ConnectController(HandleBuffer[HandleIndex],NULL,NULL,TRUE);
     }
     BS->FreePool(HandleBuffer);

     s = shell_proto->SetMap(&vfs_devpath_proto,L"zoidberg:");
      if(EFI_ERROR(s)) {
         klog("VFS",0,"SetMap failed: %d",s);
      }

}

