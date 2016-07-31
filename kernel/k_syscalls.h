#ifndef K_SYSCALLS_H
#define K_SYSCALLS_H


#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>

#ifndef ZOIDBERG_USERLAND_SDK
#include <Uefi.h>
#include <Base.h>
#include "k_thread.h"
#else
#define UINT64 uint64_t
#include <efi.h>
#include <efilib.h>
#endif

union syscall_arg {
    unsigned int fd;
    void* buf;
    char*   path;
    ssize_t count;
    int     ret_int;
    ssize_t ret_count;
    size_t  size;
    void*   ret_ptr;
    pid_t   pid;
};

typedef struct syscall_ctx {
    UINT64 task_id;
    union syscall_arg args[10];
    union syscall_arg retval;
} syscall_ctx;



#define EFI_ZOIDBERG_SYSCALL_PROTOCOL_GUID \
{ \
    0xfdb99303, 0x4eef, 0x4176, {0x99, 0x2a, 0x25, 0x1e, 0xa2, 0x64, 0x0f, 0xdb } \
}

#define ZOIDBERG_SYSCALL_PROTOCOL EFI_ZOIDBERG_SYSCALL_PROTOCOL_GUID

typedef struct _EFI_ZOIDBERG_SYSCALL_PROTOCOL EFI_ZOIDBERG_SYSCALL_PROTOCOL;

typedef EFI_ZOIDBERG_SYSCALL_PROTOCOL EFI_ZOIDBERG_SYSCALL;

/**


  @param This        pointer to the syscall protocol
  @param syscall_no  which syscall number to invoke
  @param syscall_ctx pointer to a syscall_ctx struct

  @retval EFI_SUCCESS

  To get actual return value from syscall, check ctx->retval
**/
typedef
UINT64
(EFIAPI* EFI_CALL_ZOIDBERG_SYSCALL )(
        IN EFI_ZOIDBERG_SYSCALL_PROTOCOL *This,
        IN UINT64 syscall_no,
        IN syscall_ctx* ctx
        );

struct _EFI_ZOIDBERG_SYSCALL_PROTOCOL{
    UINT64 Revision;
    UINT64 my_task_id; // TODO - make this a full struct with details on the task
    EFI_CALL_ZOIDBERG_SYSCALL call_syscall;
};

#ifndef ZOIDBERG_USERLAND
void sys_exit(struct syscall_ctx  *ctx);
void sys_read(struct syscall_ctx  *ctx);
void sys_write(struct syscall_ctx *ctx);
void sys_exec(struct syscall_ctx *ctx);
void sys_spawn(struct syscall_ctx *ctx);
void sys_zmalloc(struct syscall_ctx *ctx);
void sys_zrealloc(struct syscall_ctx *ctx);
void sys_zfree(struct syscall_ctx *ctx);
void sys_uname(struct syscall_ctx *ctx);
void sys_wait(struct syscall_ctx *ctx);

static void (*syscalls[10])(struct syscall_ctx *ctx) = {
    NULL,
    &sys_exit,
    &sys_read,
    &sys_write,
    &sys_exec,
    &sys_spawn,
    &sys_zmalloc,
    &sys_zfree,
    &sys_zrealloc,
    &sys_uname,
    &sys_wait
};
#endif

#define ZSYSCALL_EXIT    1
#define ZSYSCALL_READ    2
#define ZSYSCALL_WRITE   3
#define ZSYSCALL_EXEC    4
#define ZSYSCALL_SPAWN   5
#define ZSYSCALL_MALLOC  6
#define ZSYSCALL_FREE    7
#define ZSYSCALL_REALLOC 8
#define ZSYSCALL_UNAME   9
#define ZSYSCALL_WAIT    10

extern EFI_GUID gEfiZoidbergSyscallProtocolGUID;


#ifndef ZOIDBERG_USERLAND_SDK
void install_syscall_protocol(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable,UINT64 task_id);
#endif

#endif
