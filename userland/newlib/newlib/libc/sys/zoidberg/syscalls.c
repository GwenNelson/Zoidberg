#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <setjmp.h>
#include <stdint.h>

#include "syscalls.h"

// this is just to make libgcc happy
int atexit(void (*function)(void)) {
}

extern jmp_buf proc_start_env;
void _exit() { 
     sys_exit();
     longjmp(proc_start_env,1);
}

int close(int file) { }
char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env) { return 0; }

int spawn(char* path) {
    char* argv[] = {NULL}; // ugly hack alert
    char* envp[] = {NULL};
    return sys_spawn(path,argv,envp);
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
    return sys_getpid();
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
    return sys_read(file, ptr, len);
}

void* malloc(size_t size) {
      return sys_malloc(size);
}

void* z_malloc(size_t size) {
      return sys_malloc(size);
}

void* realloc(void* ptr, size_t size) {
      return sys_realloc(ptr, size);
}

void free(void* ptr) {
     sys_free(ptr);
}

void* sbrk_ptr=NULL;
void* sbrk_base=NULL;
uint64_t sbrk_offs=0;
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
    return sys_uname(buf);
}

int stat(const char *file, struct stat *st) { }
clock_t times(struct tms *buf) { }
int unlink(char *name) { }
int wait(int *status) { }
int write(int file, char *ptr, int len) { 
    return sys_write(file, ptr, len);
}
