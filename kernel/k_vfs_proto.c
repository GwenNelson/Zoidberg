#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include "kmsg.h"
#include "k_vfs.h"
#include <stdio.h>


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

// TODO - setup private context struct for the EFI_FILE_PROTOCOL struct
// TODO - import ext3 driver
// TODO - implement a tar driver so initrd can be a tarball
// TODO - abstraction layer for getting an EFI_FILE_PROTOCOL directly from /dev/uefi/whatever
// TODO - perhaps a gzip layer over block devices?

/*EFI_FILE_PROTOCOL ZoidbergVFSFileInterface = {
  EFI_FILE_PROTOCOL_REVISION,
  ZoidbergVFSOpen,
  ZoidbergVFSClose,
  ZoidbergVFSDelete,
  ZoidbergVFSRead,
  ZoidbergVFSWrite,
  ZoidbergVFSGetPosition,
  ZoidbergVFSSetPosition,
  ZoidbergVFSGetInfo,
  ZoidbergVFSSetInfo,
  ZoidbergVFSFlush,
  ZoidbergVFSOpenEx,
  ZoidbergVFSReadEx,
  ZoidbergVFSWriteEx,
  ZoidbergVFSFlushEx

};*/

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



EFI_SIMPLE_FILE_SYSTEM_PROTOCOL new_vfs_proto;

EFI_STATUS
EFIAPI
ZoidbergVFSOpenVolume(
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *This,
  OUT EFI_FILE_PROTOCOL                **File
  ) {
   *File = (EFI_FILE_PROTOCOL*)calloc(sizeof(EFI_FILE_PROTOCOL),1);
   if(*File == NULL) return EFI_OUT_OF_RESOURCES;
   return EFI_SUCCESS; 
}

EFI_HANDLE vfs_handle;
void init_vfs_proto() {
     klog("VFS",1,"Installing VFS protocol");
     BS->SetMem(&new_vfs_proto,sizeof(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL),0);
     new_vfs_proto.Revision   = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
     new_vfs_proto.OpenVolume = ZoidbergVFSOpenVolume;

     EFI_STATUS s = BS->InstallProtocolInterface(&vfs_handle,
                                                 &gEfiDevicePathProtocolGuid,
                                                 EFI_NATIVE_INTERFACE,
                                                 &vfs_devpath_proto);

     s = BS->InstallProtocolInterface(&vfs_handle,&gEfiSimpleFileSystemProtocolGuid,EFI_NATIVE_INTERFACE,&new_vfs_proto);
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

