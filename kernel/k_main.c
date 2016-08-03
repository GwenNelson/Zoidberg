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


#include "kmsg.h"
#include "k_thread.h"
#include "zoidberg_version.h"
#include "efiwindow/efiwindow.h"
#include "efiwindow/ewbitmap.h"
#include "libvterm/vterm.h"
#include "k_initrd.h"
#include "k_utsname.h"
#include "k_video.h"

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

// TODO - move this to another file
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



static VTerm *console_term=NULL; 
static VTermScreen* vscreen=NULL;
int term_damage(VTermRect rect, __unused void* user)
{
    /* TODO: Reimplement in a less slow-ass way */
    VTermScreenCell cell;
    VTermPos pos;
    uint8_t fg, bg, color;
    int row, col;
    for (row = rect.start_row; row < rect.end_row; row++)
    for (col = rect.start_col; col < rect.end_col; col++)
    {
        pos.col = col;
        pos.row = row;
        vterm_screen_get_cell(vscreen, pos, &cell);
//        fg = rgb2vga(cell.fg.red, cell.fg.green, cell.fg.blue);
//        bg = rgb2vga(cell.bg.red, cell.bg.green, cell.bg.blue);

        if (cell.attrs.reverse)
            color = bg | (fg << 4);
        else
            color = fg | (bg << 4);
//        display_put(col, row, cell.chars[0], color);
    }
    return 1;
}

static VTermScreenCallbacks vtsc =
{
    .damage = &term_damage,
    .moverect = NULL,
    .movecursor = NULL,
    .settermprop = NULL,
    .bell = NULL,
    .resize = NULL,
};

int main(int argc, char** argv) {

    ST = gST;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    

    init_static_kmsg();

    int i;

    char* initrd_path = NULL;
    char* vgamode     = NULL;

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
    kprintf("\t\tKernel entry point located at %#11x\n\n\n\n", (UINT64)main);
 

    draw_logo();
 
    init_dynamic_kmsg();
 

    if(initrd_path==NULL) {
       klog("INITRD",0,"No initrd= option specified!");
    } else {
       mount_initrd(initrd_path);
    }

    vfs_init();

    klog("UEFI",1,"Disabling watchdog");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    klog("TASKING",1,"Starting multitasking");
    scheduler_start();

    klog("TASKING",1,"Spawning kernel idle task");
    init_kernel_task(&idle_task,NULL);
    BS->Stall(1000);

    console_term = vterm_new(80,25);    

    if(console_term==NULL) {
       klog("TERM",0,"Failed to create libvterm terminal");
    } else {
       klog("TERM",1,"Created libvterm terminal");
       vscreen = vterm_obtain_screen(console_term);
       vterm_screen_set_callbacks(vscreen, &vtsc, NULL);
       vterm_screen_enable_altscreen(vscreen, 1);
       vterm_screen_reset(vscreen, 1);
    }

    klog("INIT",1,"Starting PID 1 /sbin/init");

 
    req_task(&uefi_run,(void*)L"initrd:\\sbin\\init");
    while(1) {
       init_tasks();
    }
    return EFI_SUCCESS;
}
