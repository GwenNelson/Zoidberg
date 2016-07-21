#ifndef K_THREAD_H
#define K_THREAD_H

#include "../SimpleThread/SimpleThread.h"

typedef struct task_def_t {
   int task_id;
   void (*task_proc)(void* arg, UINT64 task_id);
   void* ctx;
   void* arg;
   struct task_def_t *next;
   struct task_def_t *prev;
} task_def_t;

void req_task(void (*task_proc)(void* ctx), void* arg);    // request a task init
UINT64 init_task(void (*task_proc)(void* ctx), void* arg); // actually init the task (from main thread only
void init_tasks();                                         // do pending req_task
struct task_def_t *get_task(UINT64 task_id);
void kill_task(UINT64 task_id);
void scheduler_start();

#endif
