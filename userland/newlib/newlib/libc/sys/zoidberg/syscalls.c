#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>

#define ZOIDBERG_USERLAND_SDK
#include "k_syscalls.h"

extern EFI_ZOIDBERG_SYSCALL_PROTOCOL *syscall_proto;
// this is just to make libgcc happy
int atexit(void (*function)(void)) {
}

extern jmp_buf proc_start_env;
void _exit() { 
     syscall_ctx sys_ctx;
     syscall_proto->call_syscall(syscall_proto,ZSYSCALL_EXIT,&sys_ctx);
     longjmp(proc_start_env,1);
}

int close(int file) { }
char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env) { return 0; }

int vfork() { 
    errno = ENOSYS;
    return -1;
}

int fstat(int file, struct stat *st) { }
int getpid() { 
    return syscall_proto->my_task_id;
}
int isatty(int file) { }
int kill(int pid, int sig) { }
int link(char *old, char *new) { }
int lseek(int file, int ptr, int dir) { }
int open(const char *name, int flags, ...) { }
int read(int file, char *ptr, int len) { 
    syscall_ctx sys_ctx;
    sys_ctx.args[0].fd    = file;
    sys_ctx.args[1].buf   = (void*)ptr;
    sys_ctx.args[2].count = (ssize_t)len;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_READ,&sys_ctx);
    return sys_ctx.retval.ret_count;
}

caddr_t sbrk(int incr) { 
    errno = ENOSYS;
    return (caddr_t)-1;
}

int stat(const char *file, struct stat *st) { }
clock_t times(struct tms *buf) { }
int unlink(char *name) { }
int wait(int *status) { }
int write(int file, char *ptr, int len) { 
    syscall_ctx sys_ctx;
    sys_ctx.args[0].fd     = file;
    sys_ctx.args[1].buf    = (void*)ptr;
    sys_ctx.args[2].count  = (ssize_t)len;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_WRITE,&sys_ctx);
    return sys_ctx.retval.ret_count;
}
