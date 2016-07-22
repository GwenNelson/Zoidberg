#include "k_syscalls.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "kmsg.h"

EFI_GUID gEfiZoidbergSyscallProtocolGUID = EFI_ZOIDBERG_SYSCALL_PROTOCOL_GUID;

void sys_exit(struct syscall_ctx *ctx) {
}

// this is basically up to the VM to implement
void sys_fork(struct syscall_ctx *ctx) {
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

extern EFI_BOOT_SERVICES *BS;
extern EFI_HANDLE gImageHandle;

void install_syscall_protocol(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable,UINT64 task_id) {
     kprintf("k_syscalls: Installing syscall protocol\n");
     EFI_ZOIDBERG_SYSCALL_PROTOCOL new_syscall_proto;
     new_syscall_proto.my_task_id   = task_id;
     new_syscall_proto.call_syscall = CallSyscall;
     BS->InstallProtocolInterface(&gImageHandle,
                                  &gEfiZoidbergSyscallProtocolGUID,
                                  EFI_NATIVE_INTERFACE,
                                  &new_syscall_proto
                                  );

}

