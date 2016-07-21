#ifndef K_SYSCALLS_H
#define K_SYSCALLS_H


#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include "k_thread.h"

union syscall_arg {
    unsigned int fd;
    void* buf;
    ssize_t count;
    int     ret_int;
    ssize_t ret_count;
};

struct syscall_ctx {
    struct task_def_t *task;
    union syscall_arg args[10];
    union syscall_arg retval;
};

void sys_exit(struct syscall_ctx  *ctx);
void sys_fork(struct syscall_ctx  *ctx);
void sys_read(struct syscall_ctx  *ctx);
void sys_write(struct syscall_ctx *ctx);

static void *syscalls[5] = {
    NULL,
    &sys_exit,
    &sys_fork,
    &sys_read,
    &sys_write
};


#endif
