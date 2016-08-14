#include "k_syscalls.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "k_thread.h"
#include "kmsg.h"
#include "dmthread.h"
#include "k_utsname.h"
#include "k_vfs.h"

#include <sys/EfiSysCall.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/EfiShell.h>
#include <Library/BaseLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Library/DxeServicesLib.h>

#include <../MdePkg/Include/Library/DevicePathLib.h>
#include <Protocol/EfiShell.h>
#include <Library/BaseLib.h>
#include <Pi/PiFirmwareFile.h>
#include <Library/DxeServicesLib.h>
#include <IndustryStandard/Bmp.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/Cpu.h>


#include <Library/BaseLib.h>

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

void sys_exit() {
//     kill_task(ctx->task_id);
}

void conv_backslashes(CHAR16 *s)
{
        while (*s != 0)
        {
                if(*s == '/')
                        *s = '\\';
                s++;
        }
}



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
     char* _filename = (char*)_t;
     EFI_STATUS rstat = 0;
     EFI_SHELL_PROTOCOL            *shell;
     EFI_DEVICE_PATH_PROTOCOL *path;
     EFI_STATUS s = OpenShellProtocol(&shell);
     EFI_HANDLE child_h;
     path = shell->GetDevicePathFromFilePath(_filename);

     s = BS->LoadImage(0,gImageHandle,path,NULL,NULL,&child_h);
     if(EFI_ERROR(s)) {
        klog("UEFI",0,"Could not load image! Error: %d",s);
     } else {
        s = BS->StartImage(child_h,NULL,NULL);
        BS->UnloadImage(child_h);
     }
}

typedef struct spawn_req_t {
    char* wfname;
    char** argv;
    char** envp;
} spawn_req_t;

void spawn_req(void* arg) {
     task_def_t* t = (task_def_t*)arg;
     spawn_req_t *req = (spawn_req_t*)t->arg;
     char* spawn_filename = calloc(1024,1);
     wcstombs(spawn_filename,req->wfname,1024);
     klog("SPAWN",1,"Trying to spawn %s",spawn_filename);
     free(spawn_filename);
     uefi_run((void*)req->wfname);
     free(req);
}

// pid_t spawn(char* path)
pid_t sys_spawn(char* path, char** argv, char** envp) {
     klog("SPAWN",1,"Trying to spawn %s",path);
     CHAR16 *wfname = (CHAR16 *)malloc((strlen(path) + 1) * sizeof(CHAR16));
     mbstowcs((wchar_t *)wfname, path, strlen(path) + 1);
     conv_backslashes(wfname);
     spawn_req_t* req = calloc(sizeof(spawn_req_t),1);
     req->wfname = wfname;
     req->argv   = argv;
     req->envp   = envp;
     req_task(&spawn_req,(void*)req);
     // TODO - fix the memory leak without early free()
  //   free(wfname);
}

void sys_null() { }

pid_t sys_getpid() {
      return get_cur_task();
}

// ssize_t read(unsigned int fd, char* buf, size_t count)
ssize_t sys_read(unsigned int fd, void* buf, size_t count) {
      return read(fd,buf,count);
}

// ssize_t write(int fd, void* buf, uint32 count)
ssize_t sys_write(unsigned int fd, void* buf, size_t count) {
      // TODO implement multiple terminals etc, different stdin/stdout for different processes
      // TODO map the fd for the calling process
      return write(fd,buf,count);
}

void* sys_malloc(size_t size) {
      return malloc(size);
}

void sys_free(void* ptr) {
     free(ptr);
}

void* sys_realloc(void* ptr, size_t size) {
      return realloc(ptr,size);
}

int sys_uname (struct utsname *buf) {
     *buf = zoidberg_uname;
    return 0;
}

pid_t sys_wait(int* status) {
}


int sys_execve(char *filename, char **argv, char** envp) {
    CHAR16 *wfname = (CHAR16 *)malloc((strlen(filename) + 1) * sizeof(CHAR16));
    mbstowcs((wchar_t *)wfname, filename, strlen(filename) + 1);
    conv_backslashes(wfname);
    uefi_run((void*)wfname);

    return 0;
}

void EFIAPI syscall_inter_handler(IN CONST EFI_EXCEPTION_TYPE InterruptType, IN CONST EFI_SYSTEM_CONTEXT SystemContext) {
     if(SystemContext.SystemContextX64->Rax == 666) {
        SystemContext.SystemContextX64->Rax = 42;
        return;
     }
     UINT64 retval;
     UINT64 (*teh_syscall)(UINT64 a, UINT64 b, UINT64 c) = syscalls[SystemContext.SystemContextX64->Rax];
     // this is a crazy hack due to ABI differences
     retval = teh_syscall(             SystemContext.SystemContextX64->Rcx,
             SystemContext.SystemContextX64->Rdx,
             SystemContext.SystemContextX64->R8);
     SystemContext.SystemContextX64->Rax = retval;
}

void cpu_proto_init() {
     EFI_CPU_ARCH_PROTOCOL* cpu_proto;
     EFI_STATUS s = BS->LocateProtocol(&gEfiCpuArchProtocolGuid,NULL,&cpu_proto);
     if(EFI_ERROR(s)) {
        klog("CPU",0,"Could not open arch protocol!");
        return;
     } else {
        klog("CPU",1,"Opened arch protocol!");
     }

     s = cpu_proto->RegisterInterruptHandler(cpu_proto,0x80,syscall_inter_handler);
     if(EFI_ERROR(s)) {
        klog("CPU",0,"Could not register 0x80 handler: %d",s);
     } else {
        klog("CPU",1,"Registered handler!");
     }

     int a=0;
     __asm__("mov $666, %%rax;"
             "int $0x80;"
             :"=a"(a)::);
     klog("CPU",1,"Got back %d from syscall",a);
}


