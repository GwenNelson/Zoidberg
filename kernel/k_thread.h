#ifndef K_THREAD_H
#define K_THREAD_H

#include "../SimpleThread/SimpleThread.h"



struct task_def_t {
   int task_id;
   void (*task_proc)(void* arg, UINT64 task_id);
   void* ctx;
   void* arg;
};

UINT64 init_task(void (*task_proc)(void* arg, UINT64 task_id), void* arg);
struct task_def_t *get_task(UINT64 task_id);
void kill_task(UINT64 task_id);
void scheduler_start();

#endif
