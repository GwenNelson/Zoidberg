/* note these headers are all provided by newlib - you don't need to provide them */
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include "efilibc.h"

typedef struct ZOIDBERG_FILE
{
       EFI_FILE *f;
       int eof;
       int error;
       int fileno;
       int istty;
       int ttyno;
} ZOIDBERG_FILE;

ZOIDBERG_FILE* zoidberg_fromfd(int fd);

// this is just to make libgcc happy
int atexit(void (*function)(void)) {
}
 
void _exit() {
}

char **environ; /* pointer to array of char * strings that define the current environment variables */
int execve(char *name, char **argv, char **env) {
}
int fork() {
    return 0;
}
int fstat(int file, struct stat *st) {

}
int getpid() {
    return 0;
}
int isatty(int file) {
    
}
int kill(int pid, int sig) {
    return 0;
}
int link(char *old, char *new) {
    return 0;
}
int lseek(int file, int ptr, int dir) {
}

ZOIDBERG_FILE *zoidberg_fopen(const char *path, const char *mode);

int open(const char *name, int flags, ...) {
    char access_mode[3];
    if((flags & O_RDONLY) != 0) {
       access_mode[0]='r';
       access_mode[1]=0;
    } else if((flags & O_WRONLY) != 0) {
       access_mode[0]='w';
       if((flags & O_CREAT) != 0) {
          access_mode[1]='+';
          access_mode[2]=0; 
       } else {
          access_mode[1]=0;
       }
    }
    ZOIDBERG_FILE* fd = zoidberg_fopen(name,(const char*)access_mode);
    if(fd==NULL) return -1;
    return fd->fileno;
}
int close(int file) {
    ZOIDBERG_FILE* fd =zoidberg_fromfd(file);
    return zoidberg_fclose(fd);
}
int read(int file, char *ptr, int len) {
    ZOIDBERG_FILE* fd=zoidberg_fromfd(file);
    return zoidberg_fread(ptr,1,len,fd);
}
caddr_t sbrk(int incr) {}
int stat(const char *file, struct stat *st) {
}
clock_t times(struct tms *buf) {
}
int unlink(char *name) {
}
int wait(int *status) {
}
int write(int file, char *ptr, int len) {
    ZOIDBERG_FILE* fd =zoidberg_fromfd(file);
    return zoidberg_fwrite(ptr,1,len,fd);
}
