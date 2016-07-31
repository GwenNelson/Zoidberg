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
int spawn(char* path) {
    syscall_ctx sys_ctx;
    sys_ctx.args[0].path = path;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_SPAWN,&sys_ctx);
    return sys_ctx.retval.ret_int;
}

int fstat(int file, struct stat *st) { 
    if(file <= 2) {
      st->st_mode    = S_IFCHR;
      st->st_blksize = 0;
      return 0;
    }
    errno = EBADF;
    return -1;
}
int getpid() { 
    return syscall_proto->my_task_id;
}
int isatty(int file) {
    if(file <= 2) return 1;
    errno = EINVAL;
    return 0;
}
int kill(int pid, int sig) { }
int link(char *old, char *new) { }
int lseek(int file, int ptr, int dir) { 
    if(file <= 2) {
       errno = ESPIPE;
       return -1;
    }
}
int open(const char *name, int flags, ...) { }
int read(int file, char *ptr, int len) { 
    syscall_ctx sys_ctx;
    sys_ctx.args[0].fd    = file;
    sys_ctx.args[1].buf   = (void*)ptr;
    sys_ctx.args[2].count = (ssize_t)len;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_READ,&sys_ctx);
    return sys_ctx.retval.ret_count;
}

void* malloc(size_t size) {
    syscall_ctx sys_ctx;
    sys_ctx.args[0].size = size;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_MALLOC,&sys_ctx);
    return sys_ctx.retval.ret_ptr;
}

void* z_malloc(size_t size) {
    syscall_ctx sys_ctx;
    sys_ctx.args[0].size = size;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_MALLOC,&sys_ctx);
    return sys_ctx.retval.ret_ptr;
}

void* realloc(void* ptr, size_t size) {
    syscall_ctx sys_ctx;
    sys_ctx.args[0].buf=ptr;
    sys_ctx.args[1].size = size;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_REALLOC,&sys_ctx);
    return sys_ctx.retval.ret_ptr;
}

void free(void* ptr) {
    syscall_ctx sys_ctx;
    sys_ctx.args[0].buf = ptr;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_FREE,&sys_ctx);
}

void* sbrk_ptr=NULL;
void* sbrk_base=NULL;
UINT64 sbrk_offs=0;
caddr_t sbrk(int incr) { 
    if(sbrk_ptr==NULL) sbrk_ptr = sbrk_base = z_malloc(65536);
    if((sbrk_offs+incr) > 65535) {
      errno = ENOMEM;
      return (caddr_t)-1;
    }
    caddr_t old_sbrk = (caddr_t)sbrk_ptr;
    sbrk_ptr += incr;
    sbrk_offs += incr;
    return old_sbrk;
//    errno = ENOSYS;
//    return (caddr_t)-1;
}

int uname(struct utsname *buf) {
    syscall_ctx sys_ctx;
    sys_ctx.args[0].buf = buf;
    syscall_proto->call_syscall(syscall_proto,ZSYSCALL_UNAME,&sys_ctx);
    return sys_ctx.retval.ret_int;
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
