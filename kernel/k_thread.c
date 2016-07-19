
#include <stdio.h>
#include <stdlib.h>


#include "kmsg.h"
#include "k_thread.h"

extern EFI_BOOT_SERVICES *BS;
EFI_EVENT timer_ev;


struct task_def_t tasks[4096];
uint64_t    last_task_id    = 0;
uint64_t    max_task_id     = 0;
uint64_t    cur_task_id     = 0;

void timer_func(EFI_EVENT Event, void *ctx) {
//     if(tasks[cur_task_id] == NULL) cur_task_id=0;
     cur_task_id++;
     if(cur_task_id > max_task_id) cur_task_id=1;
}

uint64_t init_task(void (*init_ctx)(void** ctx, uint64_t task_id), void (*cleanup)(void* ctx, uint64_t task_id), void (*iter_loop)(void* ctx, uint64_t task_id)) {
     uint64_t new_task_id = last_task_id+1;
     max_task_id++;
     kprintf("k_thread: init_task() Starting task ID %d at %#llx\n",new_task_id,init_ctx);

     struct task_def_t new_task;

     new_task.task_id   = new_task_id;
     new_task.init_ctx  = init_ctx;
     new_task.cleanup   = cleanup;
     new_task.iter_loop = iter_loop;
     if(init_ctx != NULL) {
        new_task.ctx = NULL;
        new_task.init_ctx(&(new_task.ctx), new_task_id);
     }
     kprintf("k_thread: init_task() Scheduling task to run\n");
     BS->SetTimer(timer_ev,TimerCancel,10);    // cheap alternative to a mutex lock
     tasks[new_task_id] = new_task;
     BS->SetTimer(timer_ev,TimerPeriodic,10);  // unlock
     return new_task_id;
}

struct task_def_t *get_task(uint64_t task_id) {
     struct task_def_t* retval = NULL;
     retval = &(tasks[task_id]);
     kprintf("k_thread: get_task() - located PID %d at %#llx\n",task_id,retval);
     return retval;
}

void kill_task(uint64_t task_id) {
     
}

void scheduler_start() {
     kprintf("k_thread: scheduler_start() Allocating task table\n");
     memset(tasks,0,sizeof(struct task_def_t)*4096);
     kprintf("k_thread: scheduler_start() Configuring timer\n");
     BS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)timer_func,NULL, &timer_ev);
     EFI_STATUS timer_stat = BS->SetTimer(timer_ev,TimerPeriodic,10);
     if(timer_stat != EFI_SUCCESS) {
         kprintf("k_thread: scheduler_start() Error configuring timer!\n");
     }

     kprintf("k_thread: scheduler_start() Multitasking started\n");
}

void tasks_run() {
     if(tasks[cur_task_id].task_id > 0) {
      tasks[cur_task_id].iter_loop(tasks[cur_task_id].ctx,cur_task_id);
     }
}



