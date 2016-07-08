/*
 *
 * Zoidberg platform support - more like UEFI + efilibc platform support right now
 *
 */

#define __PHYSICSFS_INTERNAL__
#include "physfs_internal.h"
#include "physfs_platforms.h"

#ifdef PHYSFS_PLATFORM_ZOIDBERG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <efi.h>
#include <efilib.h>

const char *__PHYSFS_platformDirSeparator = "/";

// Your kernel threading support is bad and you should feel bad
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

static char *baseDir = NULL;

// TODO - fix this to actually get it from EFI in platformInit function
char *__PHYSFS_platformCalcBaseDir(const char *argv0)
{
    char *retval = (char *) allocator.Malloc(strlen(baseDir) + 1);
    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);
    strcpy(retval, baseDir); /* calculated at init time. */
    return(retval);
} /* __PHYSFS_platformCalcBaseDir */


char *__PHYSFS_platformGetUserName(void)
{
    return(NULL);  /* (*shrug*) */
} /* __PHYSFS_platformGetUserName */


char *__PHYSFS_platformGetUserDir(void)
{
    return(__PHYSFS_platformCalcBaseDir(NULL));
} /* __PHYSFS_platformGetUserDir */

char *__PHYSFS_platformRealPath(const char *path)
{
    char *retval = (char *) allocator.Malloc(strlen(path) + 1);
    strcpy(retval,path);
    return(retval);
} /* __PHYSFS_platformRealPath */

int __PHYSFS_platformSetDefaultAllocator(PHYSFS_Allocator *a)
{
    return(0);  /* just use malloc() and friends. */
} /* __PHYSFS_platformSetDefaultAllocator */

void __PHYSFS_platformDetectAvailableCDs(PHYSFS_StringCallback cb, void *data)
{
    /* no-op on this platform. */
} /* __PHYSFS_platformDetectAvailableCDs */

int __PHYSFS_platformInit(void)
{
    return(1);  /* always succeed. */
} /* __PHYSFS_platformInit */


int __PHYSFS_platformDeinit(void)
{
    return(1);  /* always succeed. */
} /* __PHYSFS_platformDeinit */

void *__PHYSFS_platformOpenRead(const char *filename)
{
    return((void*)fopen(filename, "r+"));
} /* __PHYSFS_platformOpenRead */

void *__PHYSFS_platformOpenWrite(const char *filename)
{
    return((void*)fopen(filename, "w+"));
} /* __PHYSFS_platformOpenWrite */


void *__PHYSFS_platformOpenAppend(const char *filename)
{
    return((void*)fopen(filename,"a" ));
} /* __PHYSFS_platformOpenAppend */


PHYSFS_sint64 __PHYSFS_platformRead(void *opaque, void *buffer,
                                    PHYSFS_uint32 size, PHYSFS_uint32 count)
{
    int rc = fread(buffer, size, count, opaque);
    int max = size * count;

    BAIL_IF_MACRO(rc == -1, strerror(errno), rc);
    assert(rc <= max);

    if ((rc < max) && (size > 1))
        fseek(opaque, -(rc % size), SEEK_CUR);

    return(rc / size);
} /* __PHYSFS_platformRead */

PHYSFS_sint64 __PHYSFS_platformWrite(void *opaque, const void *buffer,
                                     PHYSFS_uint32 size, PHYSFS_uint32 count)
{
    int max = size * count;
    int rc = fwrite((void *) buffer, size, count, opaque);

    BAIL_IF_MACRO(rc == -1, strerror(errno), rc);
    assert(rc <= max);

    if ((rc < max) && (size > 1))
        fseek(opaque, -(rc % size), SEEK_CUR); /* rollback to object boundary. */

    return(rc / size);
} /* __PHYSFS_platformWrite */

int __PHYSFS_platformEOF(void *opaque)
{
    return feof(opaque);
} /* __PHYSFS_platformEOF */

int __PHYSFS_platformSeek(void *opaque, PHYSFS_uint64 pos)
{
    int retval = fseek((FILE*)opaque,pos,SEEK_SET);
    if(retval==0) return 0;
    return 1;
} /* __PHYSFS_platformSeek */

PHYSFS_sint64 __PHYSFS_platformTell(void *opaque)
{
    PHYSFS_sint64 retval;
    retval = (PHYSFS_sint64)ftell(opaque);
    return(retval);
} /* __PHYSFS_platformTell */

PHYSFS_sint64 __PHYSFS_platformFileLength(void *opaque)
{
    return (PHYSFS_sint64)fsize(opaque);
} /* __PHYSFS_platformFileLength */

int __PHYSFS_platformFlush(void *opaque)
{
    int retval = fflush(opaque);
    if(retval==0) return 0;
    return 1;
} /* __PHYSFS_platformFlush */


int __PHYSFS_platformClose(void *opaque)
{
    int retval = fclose(opaque);
    if(retval==0) return 0;
    return 1;
} /* __PHYSFS_platformClose */

int __PHYSFS_platformIsDirectory(const char *fname)
{
    FILE* fd = fopen(fname,"r");
    int is_dir = f_is_dir(fd);
    fclose(fd);
    return is_dir;
} /* __PHYSFS_platformIsDirectory */

char *__PHYSFS_platformCvtToDependent(const char *prepend,
                                      const char *dirName,
                                      const char *append)
{
    int len = ((prepend) ? strlen(prepend) : 0) +
              ((append) ? strlen(append) : 0) +
              strlen(dirName) + 1;
    char *retval = (char *) allocator.Malloc(len);

    BAIL_IF_MACRO(retval == NULL, ERR_OUT_OF_MEMORY, NULL);

    /* platform-independent notation is Unix-style already.  :)  */

    if (prepend)
        strcpy(retval, prepend);
    else
        retval[0] = '\0';

    strcat(retval, dirName);

    if (append)
        strcat(retval, append);

    return(retval);
} /* __PHYSFS_platformCvtToDependent */

void __PHYSFS_platformEnumerateFiles(const char *dirname,
                                     int omitSymLinks,
                                     PHYSFS_EnumFilesCallback callback,
                                     const char *origdir,
                                     void *callbackdata)
{   
    
    FILE* dir_fd = fopen(dirname,"r");
    if(dir_fd == NULL) return;
    if(f_is_dir(dir_fd)==0) {
       fclose(dir_fd);
       return;
    }

    EFI_FILE_INFO* FileInfo = allocator.Malloc(sizeof(EFI_FILE_INFO));
    UINTN FileInfoSize = sizeof(EFI_FILE_INFO);

    char entry_filename[1024];
    while(FileInfoSize > 0) {
       EFI_STATUS s = ((_FILE*)dir_fd)->f->Read( ((_FILE*)dir_fd)->f, &FileInfoSize, FileInfo);
       if(s == EFI_SUCCESS) {
            wcstombs(entry_filename,FileInfo->FileName,1024);
            printf("entry_filename: %s\n",entry_filename);
            if (strcmp(entry_filename, ".") == 0) {
            } else if (strcmp(entry_filename, "..") == 0) {
            } else {
              callback(callbackdata, origdir, entry_filename);
            }

       } else if(s == EFI_BUFFER_TOO_SMALL) {
            FileInfo = allocator.Realloc((void*)FileInfo,FileInfoSize);
       } 

    }
    allocator.Free(FileInfo);
    fclose(dir_fd);
    
} /* __PHYSFS_platformEnumerateFiles */

int __PHYSFS_platformIsSymLink(const char *fname)
{
    return 0; // there are no symlinks in EFI
} /* __PHYSFS_platformIsSymlink */

int __PHYSFS_platformDelete(const char *path)
{
    BAIL_IF_MACRO(remove(path) == -1, strerror(errno), 0);
    return(1);
} /* __PHYSFS_platformDelete */

int __PHYSFS_platformMkDir(const char *path)
{
    int rc;
    errno = 0;
    rc = mkdir(path, 0);
    BAIL_IF_MACRO(rc == -1, strerror(errno), 0);
    return(1);
} /* __PHYSFS_platformMkDir */

PHYSFS_sint64 __PHYSFS_platformGetLastModTime(const char *fname)
{
    FILE* fd = fopen(fname,"r");
    PHYSFS_sint64 retval = (PHYSFS_sint64)f_mod_time(fd);
    fclose(fd);
    return retval;
    
} /* __PHYSFS_platformGetLastModTime */

#endif
