#ifndef K_THREAD_H
#define K_THREAD_H

#include "uthash.h"

struct task_def_t {
   UT_hash_handle hh;
   uint64_t task_id;
   void (*init_ctx)(void* ctx, uint64_t task_id);
   void (*cleanup)(void* ctx, uint64_t task_id);
   void (*iter_loop)(void* ctx, uint64_t task_id);
   void* ctx;
} task_def_t;

uint64_t init_task(void (*init_ctx)(void* ctx, uint64_t task_id), void (*cleanup)(void* ctx, uint64_t task_id), void (*iter_loop)(void* ctx, uint64_t task_id));
struct task_def_t *get_task(uint64_t task_id);
void kill_task(uint64_t task_id);
void scheduler_start();
void tasks_run();

#endif
