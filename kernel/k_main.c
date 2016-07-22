#include <stdio.h>
#include <stdlib.h>

#include <sys/EfiSysCall.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <../MdePkg/Include/Library/DevicePathLib.h>
#include <Protocol/EfiShell.h>

#include "kmsg.h"
#include "k_thread.h"
#include "zoidberg_version.h"
#include "vm_pawn.h"
#include "vm_duktape.h"

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
"*****************************************************\n\n";

EFI_STATUS OpenShellProtocol( EFI_SHELL_PROTOCOL            **gEfiShellProtocol )
{
    EFI_STATUS                      Status;
    Status = gBS->OpenProtocol(
            gImageHandle,
            &gEfiShellProtocolGuid,
            (VOID **)gEfiShellProtocol,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
    if (EFI_ERROR(Status)) {
    //
    // Search for the shell protocol
    //
        Status = gBS->LocateProtocol(
                &gEfiShellProtocolGuid,
                NULL,
                (VOID **)gEfiShellProtocol
                );
        if (EFI_ERROR(Status)) {
            gEfiShellProtocol = NULL;
        }
  }
  return Status;
}

void uefi_run(void* _t) {
     struct task_def_t *t = (struct task_def_t*)_t;
     char* _filename = (char*)t->arg;
     EFI_STATUS rstat = 0;
     EFI_SHELL_PROTOCOL            *shell;
     EFI_DEVICE_PATH_PROTOCOL *path;
     EFI_STATUS s = OpenShellProtocol(&shell);
     EFI_HANDLE child_h;
     path = shell->GetDevicePathFromFilePath(_filename);

     s = BS->LoadImage(0,gImageHandle,path,NULL,NULL,&child_h);
     kprintf("%d\n",s);
     install_syscall_protocol(child_h,ST,t->task_id);
     s = BS->StartImage(child_h,NULL,NULL);
     kprintf("%d\n",s);
}
 
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
    
    init_task(&uefi_run,(void*)L"initrd:\\sbin\\init");
    init_task(&uefi_run,(void*)L"fs0:\\task_a.efi");
    init_task(&uefi_run,(void*)L"fs0:\\task_b.efi");
    while(1) {
       BS->Stall(1000);
       init_tasks();
    }
    return EFI_SUCCESS;
}
