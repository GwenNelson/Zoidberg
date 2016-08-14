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


extern void uefi_run(void* _t);
// pid_t spawn(char* path)
pid_t sys_spawn(char* path) {
     CHAR16 *wfname = (CHAR16 *)malloc((strlen(path) + 1) * sizeof(CHAR16));
     mbstowcs((wchar_t *)wfname, path, strlen(path) + 1);
     conv_backslashes(wfname);
     req_task(&uefi_run,(void*)wfname);
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


