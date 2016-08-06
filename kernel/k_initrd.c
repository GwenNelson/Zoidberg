
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
EFI_HANDLE               *initrd_handle=NULL;

#define INITRD_BLOCKSIZE 512

EFI_STATUS EFIAPI InitRDReadBlocks(
	IN EFI_BLOCK_IO *This,
	IN UINT32       MediaId,
	IN EFI_LBA      LBA,
	IN UINTN        BufferSize,
	OUT VOID        *Buffer)
{
	void* block_ptr;
	EFI_BLOCK_IO_MEDIA *Media = This->Media;
  if(BufferSize % Media->BlockSize != 0)
                return EFI_BAD_BUFFER_SIZE;

        if(LBA > Media->LastBlock)
                return EFI_DEVICE_ERROR;

        if(LBA + BufferSize / Media->BlockSize - 1 > Media->LastBlock)
                return EFI_DEVICE_ERROR;


        block_ptr = initrd_buf + MultU64x32(LBA,INITRD_BLOCKSIZE);
	memcpy(Buffer,block_ptr,BufferSize);
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI InitRDWriteBlocks(
	IN EFI_BLOCK_IO *This,
	IN UINT32       MediaId,
	IN EFI_LBA      LBA,
	IN UINTN        BufferSize,
	IN VOID         *Buffer)
{
	return EFI_WRITE_PROTECTED;
}

EFI_STATUS EFIAPI InitRDFlushBlocks(
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


int mount_initrd(char* path) {
     klog("INITRD",1,"Opening image in %s",path);
     FILE* fd = fopen(path,"rb");
     if(fd==NULL) {
        klog("INITRD",0,"Failed to open initrd image");
        return 1;
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
        return 1;
     }

     klog("INITRD",KLOG_PROG,"Reading image into memory");
     kmsg_prog_start(size);

     void* cur_buf = initrd_buf; 
     int i=0;
     size_t retval=0;
     size_t read_so_far=0;
     while(read_so_far < size) {
         retval = fread(cur_buf,512,8,fd);
         if(retval != EOF) {
            cur_buf     += retval*512;
            read_so_far += retval*512;
            kmsg_prog_update(retval*512);
         }
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
       return 1;
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


     UINTN HandleCount;
     EFI_HANDLE *HandleBuffer;
     UINTN HandleIndex;
     s = BS->LocateHandleBuffer(AllHandles,NULL,NULL,&HandleCount,&HandleBuffer);
     for(HandleIndex=0; HandleIndex < HandleCount; HandleIndex++) {
         BS->ConnectController(HandleBuffer[HandleIndex],NULL,NULL,TRUE);
     }
     BS->FreePool(HandleBuffer);


      s = shell_proto->SetMap(&initrd_devpath_proto,L"initrd:");
      if(EFI_ERROR(s)) {
         klog("INITRD",0,"SetMap failed: %d",s);
         return 1;
      }
      return 0;
}
