#ifndef UTSNAME_H
#define UTSNAME_H

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

#ifndef IN_UTSNAME
extern struct utsname zoidberg_uname;
#endif

int uname(struct utsname *buf);

#endif
