#include <efi.h>
#include <efilib.h>

#define ZOIDBERG_USERLAND_SDK
#include "k_syscalls.h"

extern int main();

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_RUNTIME_SERVICES *RT;
EFI_HANDLE gImageHandle;
EFI_ZOIDBERG_SYSCALL_PROTOCOL *syscall_proto = NULL;
EFI_GUID gEfiZoidbergSyscallProtocolGUID = EFI_ZOIDBERG_SYSCALL_PROTOCOL_GUID;

EFI_STATUS
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
     ST = SystemTable;
     BS = ST->BootServices;
     RT = ST->RuntimeServices;
     gImageHandle = ImageHandle;
     EFI_STATUS s = BS->LocateProtocol(&gEfiZoidbergSyscallProtocolGUID , 0, (VOID**)&syscall_proto);
     if(s==0) {
       ST->ConOut->OutputString(ST->ConOut, L"crt0: Failed to connect to syscall protocol!\n\r");
       return 1;
     } else {
       int ex = main();
     }
     return 0;
}
