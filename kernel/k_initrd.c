
#include <stdio.h>
#include <stdlib.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/EfiShell.h>

#include "kmsg.h"

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

void* initrd_buf;

EFI_BLOCK_IO_PROTOCOL    initrd_proto;
EFI_BLOCK_IO_MEDIA       initrd_media;
//MEMMAP_DEVICE_PATH       initrd_devpath_proto;
EFI_HANDLE               *initrd_handle=NULL;

#define INITRD_BLOCKSIZE 512

EFI_STATUS InitRDReadBlocks(
	IN EFI_BLOCK_IO *This,
	IN UINT32       MediaId,
	IN EFI_LBA      LBA,
	IN UINTN        BufferSize,
	OUT VOID        *Buffer)
{
        void* block_ptr = initrd_buf + (LBA*INITRD_BLOCKSIZE);
        BS->CopyMem(Buffer,block_ptr,512);
	return EFI_SUCCESS;
}

EFI_STATUS InitRDWriteBlocks(
	IN EFI_BLOCK_IO *This,
	IN UINT32       MediaId,
	IN EFI_LBA      LBA,
	IN UINTN        BufferSize,
	IN VOID         *Buffer)
{
	return EFI_WRITE_PROTECTED;
}

EFI_STATUS InitRDFlushBlocks(
	IN EFI_BLOCK_IO *This)
{
	return EFI_SUCCESS;
}

typedef struct __attribute__((__packed__)) sRAM_DISK_DEVICE_PATH
{
        EFI_DEVICE_PATH Header;
        EFI_GUID        Guid;
        UINT8                   DiskId[8];
        EFI_DEVICE_PATH EndDevicePath;
} RAM_DISK_DEVICE_PATH;

RAM_DISK_DEVICE_PATH initrd_devpath_proto =
{
        MESSAGING_DEVICE_PATH,
        MSG_VENDOR_DP,
        sizeof(RAM_DISK_DEVICE_PATH) - sizeof(EFI_DEVICE_PATH),
        0,
        // {06ED4DD0-FF78-11d3-BDC4-00A0C94053D1}
        0x6ed4dd0, 0xff78, 0x11d3, 0xbd, 0xc4, 0x0, 0xa0, 0xc9, 0x40, 0x53, 0xd1,
        0,0,0,0,0,0,0,0,        // ID assigned below
        END_DEVICE_PATH_TYPE,
        END_ENTIRE_DEVICE_PATH_SUBTYPE,
        sizeof(EFI_DEVICE_PATH)
};


void mount_initrd(char* path) {
     klog("INITRD",1,"Opening image in %s",path);
     FILE* fd = fopen(path,"rb");
     if(fd==NULL) {
        klog("INITRD",0,"Failed to open initrd image");
        return;
     }
     fseek(fd,0,SEEK_END);
     long size = ftell(fd);
     klog("INITRD",1,"Image is %d bytes long",size);
     fseek(fd,0,SEEK_SET);
     fclose(fd);
     fd = fopen(path,"rb");
     
     initrd_buf = malloc(size+1);
     if(initrd_buf == NULL) {
        klog("INITRD",0,"Failed to allocate memory buffer for initrd image");
        fclose(fd);
        return;
     }

     klog("INITRD",1,"Reading image into memory");
     
     size_t retval = fread(initrd_buf,1,size,fd);
     
     if(retval != size) {
        klog("INITRD",0,"Failed to read image into memory!");
        fclose(fd);
        free(initrd_buf);
        return;
     }
    
     fclose(fd);

     klog("INITRD",1,"Installing block I/O protocol");

     initrd_proto.Revision = EFI_BLOCK_IO_INTERFACE_REVISION;
     initrd_proto.Media    = &initrd_media;

     initrd_proto.ReadBlocks  = InitRDReadBlocks;
     initrd_proto.WriteBlocks = InitRDWriteBlocks;
     initrd_proto.FlushBlocks = InitRDFlushBlocks;

     initrd_media.RemovableMedia   = FALSE;
     initrd_media.MediaPresent     = TRUE;
     initrd_media.BlockSize        = INITRD_BLOCKSIZE;
     initrd_media.LastBlock        = size/INITRD_BLOCKSIZE - 1;
     initrd_media.LogicalPartition = TRUE;
     initrd_media.ReadOnly         = TRUE;
     initrd_media.WriteCaching     = FALSE;




     EFI_STATUS s=BS->InstallProtocolInterface(&initrd_handle,
                                             &gEfiBlockIoProtocolGuid,
                                             EFI_NATIVE_INTERFACE,
                                             &initrd_proto);

     s = BS->InstallProtocolInterface(&initrd_handle,
                                                 &gEfiDevicePathProtocolGuid,
                                                 EFI_NATIVE_INTERFACE,
                                                 &initrd_devpath_proto);

     if(s==EFI_SUCCESS) {
       klog("INITRD",1,"Added protocol interface to handle %#llx", initrd_handle);
     } else {
       klog("INITRD",0,"Failed to add protocol interface, return value is %d", s);
       return;
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
        s = gBS->LocateProtocol(
                &gEfiShellProtocolGuid,
                NULL,
                &shell_proto
                );
      }
      system("fs0:\\EFI\\BOOT\\BOOTX64.EFI -nostartup");
      s = shell_proto->SetMap(&initrd_devpath_proto,L"initrd");
      if(EFI_ERROR(s)) {
         klog("INITRD",0,"SetMap failed: %d",s);
      }
}
