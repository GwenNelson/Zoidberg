#include "k_syscalls.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "k_thread.h"
#include "kmsg.h"


extern EFI_BOOT_SERVICES *BS;
EFI_GUID gEfiZoidbergSyscallProtocolGUID = EFI_ZOIDBERG_SYSCALL_PROTOCOL_GUID;

void sys_exit(struct syscall_ctx *ctx) {
     kill_task(ctx->task_id);
}

void sys_exec(struct syscall_ctx *ctx) {
}

// pid_t vfork()
void sys_vfork(struct syscall_ctx *ctx) {
     // TODO copy the thread stack here
     ctx->retval.ret_int = 0;
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

EFI_STATUS
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

