#include "k_syscalls.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include "kmsg.h"

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


