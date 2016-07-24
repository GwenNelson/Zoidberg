#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
     install_syscall_protocol(child_h,ST,t->task_id);
     s = BS->StartImage(child_h,NULL,NULL);
     BS->UnloadImage(child_h);
}

void idle_task(void* _t) {
     klog("IDLE",1,"Kernel idle task started");
     for(;;) {
         BS->Stall(100000);
     }
}
 
int main(int argc, char** argv) {

    ST = gST;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    

    init_static_kmsg();

    int i;

    char* initrd_path = NULL;

    if(argc>1) {
       for(i=1; i<argc; i++) {
           if(strncmp(argv[i],"initrd=",7)==0) {
              initrd_path = argv[i]+7;
           }
       }
    }

    ST->ConOut->ClearScreen(ST->ConOut);

    char* build_no = ZOIDBERG_BUILD;
    char* version  = ZOIDBERG_VERSION;
    ST->ConOut->SetAttribute(ST->ConOut,EFI_TEXT_ATTR(EFI_WHITE,EFI_BACKGROUND_BLACK));
    kprintf("Zoidberg kernel, Copyright 2016 Gareth Nelson\n");
    kprintf("Kernel version: %s, build number: %s\n", version, build_no);
    kprintf("Kernel entry point located at %#11x\n\n", (UINT64)main);
    
    init_dynamic_kmsg();

    if(initrd_path==NULL) {
       klog("INITRD",0,"No initrd= option specified, will default to initrd.img");
    } else {
       klog("INITRD",1,"Mounting initrd image from %s",initrd_path);
    }

    klog("UEFI",1,"Disabling watchdog");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    klog("TASKING",1,"Starting multitasking");
    scheduler_start();

    klog("TASKING",1,"Spawning kernel idle task");
    init_kernel_task(&idle_task,NULL);
    BS->Stall(1000);

    klog("INIT",1,"Starting PID 1 /sbin/init");
 
    req_task(&uefi_run,(void*)L"initrd:\\sbin\\init");
    while(1) {
       BS->Stall(100);
       init_tasks();
    }
    return EFI_SUCCESS;
}
