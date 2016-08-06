#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../k_vfs.h"
extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

#define IN_DEVUEFI
#include "devuefi.h"

vfs_fs_type_t *devuefi_fs_type = NULL;
char* dev_uefi_fs_type_s = "devuefi";

char** vfs_devuefi_list_root_dir(vfs_fs_handler_t* this) {
    UINTN BufferSize=0;
    EFI_HANDLE *HandleBuffer;

    EFI_STATUS s = BS->LocateHandle(ByProtocol,&gEfiBlockIoProtocolGuid,NULL,&BufferSize,HandleBuffer);
    if(s == EFI_BUFFER_TOO_SMALL) {
       HandleBuffer = (EFI_HANDLE*)calloc(BufferSize,1);
       s = BS->LocateHandle(ByProtocol,&gEfiBlockIoProtocolGuid,NULL,&BufferSize,HandleBuffer);
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


    char** retval = (char**)calloc(sizeof(char*),(BufferSize/(sizeof(EFI_HANDLE))));

    int i=0;
    int retval_i=0;
    CHAR16* dev_name;
    char mapping_name[128];
    char *single_map;
    for(i=0; i< (BufferSize / sizeof(EFI_HANDLE)); i++) {
        EFI_DEVICE_PATH_PROTOCOL *dev_path;
        s = BS->OpenProtocol(HandleBuffer[i],&gEfiDevicePathProtocolGuid,&dev_path,gImageHandle,NULL,EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if(s==EFI_SUCCESS) {
           dev_name = NULL;
           dev_name = shell_proto->GetMapFromDevicePath(&dev_path);
           if(dev_name != NULL) {
              retval[i] = calloc(sizeof(char),128);
              wcstombs(retval[i],dev_name,128);
              retval[i][strcspn(retval[i],":;")]=0;
           }
        }
        retval[i+1] = NULL;
    }
    return retval;

}




// we don't actually care about the dev_name or mountpoint here, but other drivers will care
void vfs_devuefi_setup(vfs_fs_handler_t* this, char* dev_name, char* mountpoint) {
     // we don't actually need to set anything here except list_root_dir and file_exists
     // other fields are either optional or already set in k_vfs.c (specifically fs_type and setup)
     this->list_root_dir = &vfs_devuefi_list_root_dir;

     // TODO - later, other methods should be implemented here to do block I/O
}

void vfs_init_devuefi_fs_type() {
     devuefi_fs_type          = (vfs_fs_type_t*)calloc(sizeof(vfs_fs_type_t),1);
     devuefi_fs_type->fs_type = dev_uefi_fs_type_s;
     devuefi_fs_type->setup   = &vfs_devuefi_setup;
     klog("VFS",1,"devuefi filesystem driver setup");
}
