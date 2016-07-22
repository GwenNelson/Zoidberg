#include <efi.h>
#include <efilib.h>

extern void exit(int code);
extern int main();

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
     int ex = main();
     exit(ex);
}
