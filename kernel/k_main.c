#include <stdio.h>
#include <stdlib.h>

#include <sys/EfiSysCall.h>
#include <Library/UefiBootServicesTableLib.h>

#include "kmsg.h"
#include "k_thread.h"
#include "zoidberg_version.h"
#include "vm_pawn.h"

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_RUNTIME_SERVICES *RT;
extern EFI_HANDLE gImageHandle;

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
 
int main(int argc, char** argv) {
    if(argc>1) {
    }
    ST = gST;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    

    init_static_kmsg();

    kprintf(why_not_header);

    char* build_no = ZOIDBERG_BUILD;
    kprintf("Zoidberg kernel, build %s booting\n", build_no);
    kprintf("Kernel entry point (UefiMain) located at: %#11x\n", (UINT64)main);
    
    init_dynamic_kmsg();

    kprintf("Disabling UEFI Watchdog\n");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    kprintf("Starting multitasking\n");
    scheduler_start();

    kprintf("Starting PID 1 /sbin/init\n");
    UINT64 init_pid = init_task(&vm_pawn_mainproc,(void*)"initrd:\\sbin\\init");

    while(1) {
       BS->Stall(1000);
       init_tasks();
    }
    return EFI_SUCCESS;
}