#include <efi.h>
#include <efilib.h>

#include <stdio.h>
#include <stdlib.h>
#include "uthash.h"

#include "kmsg.h"
#include "k_thread.h"

extern EFI_BOOT_SERVICES *BS;
EFI_EVENT timer_ev;


struct task_def_t *tasks    = NULL;
uint64_t    last_task_id    = 0;
struct task_def_t *cur_task = NULL;

void timer_func(EFI_EVENT Event, void *ctx) {
     if(cur_task==NULL) { 
        cur_task=tasks;
        return;
     }
     if(cur_task->hh.next == NULL) {
        cur_task = tasks;
     } else {
        cur_task = cur_task->hh.next;
     }
}

void init_task(void (*init_ctx)(void* ctx), void (*cleanup)(void* ctx), void (*iter_loop)(void* ctx)) {
     uint64_t new_task_id = last_task_id+1;
     kprintf("k_thread: init_task() Starting task ID %d at %#llx\n",new_task_id,init_ctx);

     struct task_def_t *new_task = malloc(sizeof(task_def_t));

     new_task->task_id   = new_task_id;
     new_task->init_ctx  = init_ctx;
     new_task->cleanup   = cleanup;
     new_task->iter_loop = iter_loop;
     if(init_ctx != NULL) {
        new_task->init_ctx(new_task->ctx);
     }
     BS->SetTimer(timer_ev,TimerCancel,10);    // cheap alternative to a mutex lock
     HASH_ADD_INT(tasks, task_id, new_task);
     BS->SetTimer(timer_ev,TimerPeriodic,10);  // unlock
}

void kill_task(uint64_t task_id) {
     
}

void scheduler_start() {
     kprintf("k_thread: scheduler_start() Configuring timer\n");
     BS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)timer_func,NULL, &timer_ev);
     EFI_STATUS timer_stat = BS->SetTimer(timer_ev,TimerPeriodic,10);
     if(timer_stat != EFI_SUCCESS) {
         kprintf("k_thread: scheduler_start() Error configuring timer!\n");
     }
     kprintf("k_thread: scheduler_start() Multitasking started\n");
}

void tasks_run() {
     if(cur_task != NULL) cur_task->iter_loop(cur_task->ctx);
}



