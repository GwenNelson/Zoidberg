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

char why_not_header[]=""\
"*****************************************************\n"\
"*                   Why not zoidberg?               *\n"\
"*                                                   *\n"\
"* Zoidberg kernel Copyright(2016) Gareth Nelson     *\n"\
"*                                                   *\n"\
"* This program is free software, see file COPYING   *\n"\
"* for full details.                                 *\n"\
"*                                                   *\n"\
"* This program is not produced by or affiliated     *\n"\
"* with Fox or the curiosity company                 *\n"\
"*****************************************************\n\n";
 
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    gImageHandle = ImageHandle;
    
    EFI_LOADED_IMAGE *li;
 
    BS->HandleProtocol(ImageHandle, &LoadedImageProtocol, (void**)&li);
    efilibc_init(gImageHandle);

    init_static_kmsg();

    kprintf(why_not_header);

    char* build_no = ZOIDBERG_BUILD;
    kprintf("Zoidberg kernel, build %s booting\n", build_no);
    kprintf("Kernel loaded at: %#llx\n", (uint64_t)li->ImageBase);
    kprintf("Kernel entry point (efi_main) located at: %#11x\n", (uint64_t)efi_main);

    init_mem();
    
    init_dynamic_kmsg();

    kprintf("Disabling UEFI Watchdog\n");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    while(1) {
    } 
}
