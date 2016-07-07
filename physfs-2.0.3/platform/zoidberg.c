/*
 *
 * Zoidberg platform support - more like UEFI + efilibc platform support right now
 *
 */

#define __PHYSICSFS_INTERNAL__
#include "physfs_platforms.h"

#ifdef PHYSFS_PLATFORM_ZOIDBERG

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if (defined PHYSFS_NO_THREAD_SUPPORT)
void *__PHYSFS_platformGetThreadID(void) { return((void *) 0x0001); }
void *__PHYSFS_platformCreateMutex(void) { return((void *) 0x0001); }
void __PHYSFS_platformDestroyMutex(void *mutex) {}
int __PHYSFS_platformGrabMutex(void *mutex) { return(1); }
void __PHYSFS_platformReleaseMutex(void *mutex) {}
#endif

int __PHYSFS_platformExists(const char *fname)
{
    FILE* fd = fopen(fname,"r");
    int retval=0;
    if(fd==NULL) {
       if(errno==ENOENT) return retval;
       retval=1; // this is a silly assumption
    } else {
       retval=1;
       fclose(fd);
    }
    return retval;
} /* __PHYSFS_platformExists */

#endif
