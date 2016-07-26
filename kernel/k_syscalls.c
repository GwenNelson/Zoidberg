#include "k_syscalls.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "k_thread.h"
#include "kmsg.h"
#include "dmthread.h"

#include <Library/BaseLib.h>

extern EFI_BOOT_SERVICES *BS;
EFI_GUID gEfiZoidbergSyscallProtocolGUID = EFI_ZOIDBERG_SYSCALL_PROTOCOL_GUID;

void sys_exit(struct syscall_ctx *ctx) {
     kill_task(ctx->task_id);
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
void sys_spawn(struct syscall_ctx *ctx) {
     char* path = ctx->args[0].path;
     CHAR16 *wfname = (CHAR16 *)malloc((strlen(path) + 1) * sizeof(CHAR16));
     mbstowcs((wchar_t *)wfname, path, strlen(path) + 1);
     conv_backslashes(wfname);
     //UINT64 pid = init_task(&uefi_run,(void*)wfname);
     req_task(&uefi_run,(void*)wfname);
//     ctx->retval.ret_int = pid;
}

void sys_exec(struct syscall_ctx *ctx) {
}

// ssize_t read(unsigned int fd, char* buf, size_t count)
void sys_read(struct syscall_ctx *ctx) {
      unsigned int fd       = ctx->args[0].fd;
      void*   buf           = ctx->args[1].buf;
      ssize_t count         = ctx->args[2].count;
      ctx->retval.ret_count = read(fd,buf,count);
}

// ssize_t write(int fd, void* buf, uint32 count)
void sys_write(struct syscall_ctx *ctx) {
      // TODO implement multiple terminals etc, different stdin/stdout for different processes
      // TODO map the fd for the calling process
      unsigned int fd       = ctx->args[0].fd;
      void*   buf           = ctx->args[1].buf;
      ssize_t count         = ctx->args[2].count;
      ctx->retval.ret_count = write(fd,buf,count);
}

UINT64
EFIAPI
CallSyscall(
        IN EFI_ZOIDBERG_SYSCALL_PROTOCOL *This,
        IN UINT64 syscall_no,
        IN syscall_ctx* ctx)
{
        ctx->task_id = This->my_task_id;
        syscalls[syscall_no](ctx);
        return 0;
}

extern EFI_HANDLE gImageHandle;
EFI_ZOIDBERG_SYSCALL_PROTOCOL new_syscall_proto;

void install_syscall_protocol(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable,UINT64 task_id) {
     klog("SYSCALL",1,"Installing syscall protocol for task ID %d",task_id);

     new_syscall_proto.my_task_id   = task_id;
     new_syscall_proto.call_syscall = CallSyscall;
     BS->InstallProtocolInterface(&ImageHandle,
                                  &gEfiZoidbergSyscallProtocolGUID,
                                  EFI_NATIVE_INTERFACE,
                                  &new_syscall_proto
                                  );

}

