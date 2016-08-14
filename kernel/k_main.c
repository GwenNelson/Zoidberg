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
#include <Library/BaseLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Library/DxeServicesLib.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Cpu.h>

#include "kmsg.h"
#include "k_thread.h"
#include "zoidberg_version.h"
#include "efiwindow/efiwindow.h"
#include "efiwindow/ewbitmap.h"
#include "k_initrd.h"
#include "k_utsname.h"
#include "k_video.h"
#include "k_syscalls.h"

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_RUNTIME_SERVICES *RT;
extern EFI_HANDLE gImageHandle;

void idle_task(void* _t) {
     klog("IDLE",1,"Kernel idle task started");
     for(;;) {
         __asm__("hlt");
//         BS->Stall(100000);
     }
}



char* argv0; // this needs to be exported for the sake of the VFS module

void userland_init(void* arg) {
     // this is a bit of a cheat (directly invoking a syscall function) - will need to use inline asm later
     char* argv[]={"/sbin/init",NULL};
     char* env[] ={"PATH=/bin:/sbin",NULL};
     sys_execve("initrd:/sbin/init",argv,env);
}

int main(int argc, char** argv) {

    ST = gST;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;

    init_static_kmsg();

    int i;

    char* initrd_path = NULL;
    char* vgamode     = NULL;

    argv0 = argv[0];
    if(argc>1) {
       for(i=1; i<argc; i++) {
           if(strncmp(argv[i],"initrd=",7)==0) {
              initrd_path = argv[i]+7;
           } else if(strncmp(argv[i], "vgamode=",8)==0) {
              vgamode = argv[i]+8;
           }
       }
    }

    init_video(vgamode);
    ST->ConOut->ClearScreen(ST->ConOut);

    char* build_no = ZOIDBERG_BUILD;
    char* version  = ZOIDBERG_VERSION;
    ST->ConOut->SetAttribute(ST->ConOut,EFI_TEXT_ATTR(EFI_WHITE,EFI_BACKGROUND_BLACK));
    kprintf("\n");
    kprintf("\t\tZoidberg kernel, Copyright 2016 Gareth Nelson\n");
    kprintf("\t\t%s %s %s %s\n",zoidberg_uname.sysname, zoidberg_uname.release, zoidberg_uname.version, zoidberg_uname.machine);
    kprintf("\t\t%s entry point located at %#11x\n\n\n\n", argv0, (UINT64)main);

    draw_logo();

    init_console();
 
    init_dynamic_kmsg();

    cpu_proto_init();

    vfs_init(); 

    if(initrd_path==NULL) {
       klog("INITRD",0,"No initrd= option specified!");
       klog("INITRD",0,"Can not continue without a valid initrd.img, startup aborted!");
       return;
    } else {
       if (mount_initrd(initrd_path) != 0) {
           klog("INITRD",0,"Startup aborted!");
           return;
       }
    }

    dump_vfs();

    klog("UEFI",1,"Disabling watchdog");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    klog("TASKING",1,"Starting multitasking");
    scheduler_start();

    klog("TASKING",1,"Spawning kernel idle task");
    init_kernel_task(&idle_task,NULL);
    BS->Stall(1000);

    
    klog("INIT",1,"Starting PID 1 /sbin/init");

  //  system("fs0:\\EFI\\BOOT\\BOOTX64.efi -nostartup -nomap");
 
    req_task(&userland_init,NULL);
    while(1) {
       init_tasks();
    }
    return EFI_SUCCESS;
}
