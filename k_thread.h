#ifndef K_THREAD_H
#define K_THREAD_H

#include "uthash.h"

struct task_def_t {
   UT_hash_handle hh;
   uint64_t task_id;
   void (*init_ctx)(void* ctx);
   void (*cleanup)(void* ctx);
   void (*iter_loop)(void* ctx);
   void* ctx;
} task_def_t;

void init_task(void (*init_ctx)(void* ctx), void (*cleanup)(void* ctx), void (*iter_loop)(void* ctx));
void kill_task(uint64_t task_id);
void scheduler_start();
void tasks_run();

#endif
