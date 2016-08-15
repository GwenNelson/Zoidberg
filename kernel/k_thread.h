#ifndef K_THREAD_H
#define K_THREAD_H

#include "dmthread.h"
#include <sys/EfiSysCall.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DevicePath.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>

typedef struct task_def_t {
   int task_id;
   void (*task_proc)(void* arg, UINT64 task_id);
   thread_list* ctx;
   void* arg;

   char* environ;
   char* cwd;

   struct task_def_t *next;
   struct task_def_t *prev;
} task_def_t;

UINT64 get_cur_task();

// TODO - make req_task return the task ID
void req_task(void (*task_proc)(void* ctx), void* arg);    // request a task init
UINT64 init_task(void (*task_proc)(void* ctx), void* arg, UINT64 desired_id); // actually init the task (from main thread only)
void init_tasks();                                         // do pending req_task
void init_kernel_task(void (*task_proc)(void* ctx), void* arg); // init a kernel task, can NOT be killed - use with care
struct task_def_t *get_task(UINT64 task_id);
void kill_task(UINT64 task_id);
void scheduler_start();

void yield_until(EFI_EVENT e); // yield execution to other threads until the event is signalled

#endif
