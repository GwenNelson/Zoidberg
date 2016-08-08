#ifndef Z_SYSCALLS_H
#define Z_SYSCALLS_H
#include <setjmp.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>

#define _UTSNAME_LENGTH 65
#define _UTSNAME_SYSNAME_LENGTH _UTSNAME_LENGTH
#define _UTSNAME_NODENAME_LENGTH _UTSNAME_LENGTH
#define _UTSNAME_RELEASE_LENGTH _UTSNAME_LENGTH
#define _UTSNAME_VERSION_LENGTH _UTSNAME_LENGTH
#define _UTSNAME_MACHINE_LENGTH _UTSNAME_LENGTH

struct utsname {
    char sysname[_UTSNAME_SYSNAME_LENGTH];
    char nodename[_UTSNAME_NODENAME_LENGTH];
    char release[_UTSNAME_RELEASE_LENGTH];
    char version[_UTSNAME_VERSION_LENGTH];
    char machine[_UTSNAME_MACHINE_LENGTH];
};


#include "syscalls.inc"

#endif
