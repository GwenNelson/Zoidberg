#include <efi.h>
#include <efilib.h>
#include <stdlib.h>
#include "efilibc.h"

#include "kmsg.h"
#include "k_heap.h"
#include "zoidberg_version.h"

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_RUNTIME_SERVICES *RT;
EFI_HANDLE gImageHandle;

 
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    gImageHandle = ImageHandle;
    
    EFI_LOADED_IMAGE *li;
 
    BS->HandleProtocol(ImageHandle, &LoadedImageProtocol, (void**)&li);
    efilibc_init(gImageHandle);

    init_static_kmsg();

    char* build_no = ZOIDBERG_BUILD;
    kprintf("Zoidberg kernel, build %s booting\n", build_no);
    kprintf("Kernel loaded at: %#llx\n", (uint64_t)li->ImageBase);
    kprintf("Kernel entry point (efi_main) located at: %#11x\n", (uint64_t)efi_main);

    init_mem();

    while(1) {
    } 
}
