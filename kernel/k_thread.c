#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>


#include <stdio.h>
#include <stdlib.h>


#include "kmsg.h"
#include "k_thread.h"
#include "dmthread.h"

extern EFI_BOOT_SERVICES *BS;

task_def_t tasks[4096];

task_def_t *pending_tasks       = NULL;
task_def_t *pending_tasks_last  = NULL;

volatile UINT8 req_tasks_lock=0; // crappy crappy mutex

UINT64    last_task_id    = 0;
UINT64    max_task_id     = 0;
UINT64    cur_task_id     = 0;

void acquire_tasks_lock() {   // warning - this motherfucker blocks FOREVER, FOR ALL ETERNITY if already locked
     while(__sync_lock_test_and_set(&req_tasks_lock, 1)) {
     }
}

void release_tasks_lock() {
     __sync_synchronize();
     req_tasks_lock=0;
}

void init_tasks() {
     acquire_tasks_lock();
     task_def_t* t = pending_tasks;
     while(t != NULL) {
       klog("TASKING",1,"Found a task_req");
       if(t != NULL) {
          UINT64 new_task_id = init_task(t->task_proc, t->arg);
          klog("TASKING",1,"task_req %d honoured",new_task_id);
       }
       if(t-> next != NULL) {
          t = t->next;
          free(t->prev);
          t->prev = NULL;
       } else {
          free(t);
          t = NULL;
       }
     }
     pending_tasks      = NULL;
     pending_tasks_last = NULL;
     release_tasks_lock();
}

void req_task(void (*task_proc)(void* ctx), void* arg) {
     acquire_tasks_lock();
     if(pending_tasks == NULL) {  // need to setup the list
        pending_tasks = (task_def_t*)malloc(sizeof(task_def_t));
        BS->SetMem((void*)pending_tasks,sizeof(task_def_t),0);
        pending_tasks->arg       = arg;
        pending_tasks->task_proc = task_proc;
        pending_tasks_last = pending_tasks;
     } else {
        task_def_t* new_task = (task_def_t*)malloc(sizeof(task_def_t));
        BS->SetMem((void*)new_task,sizeof(task_def_t),0);
        new_task->prev = pending_tasks_last;
        pending_tasks_last->next = new_task;
     }
     release_tasks_lock();
}

UINT64 init_task(void (*task_proc)(void* ctx), void* arg) {
     UINT64 new_task_id = last_task_id+1;
     if(tasks[new_task_id].task_id > 0) return;
     max_task_id++;
     klog("TASKING",1,"Starting task ID %d at %#llx",new_task_id,task_proc);

     task_def_t new_task;

     new_task.task_id       = new_task_id;
     new_task.task_proc     = task_proc;
     new_task.arg           = arg;
     tasks[new_task_id]  = new_task;
     tasks[new_task_id].ctx = create_thread((THREAD_FUNC_T)task_proc,&(tasks[new_task_id]));
     last_task_id = new_task_id; 
     return new_task.task_id;
}

void init_kernel_task(void (*task_proc)(void* ctx), void* arg) {
     klog("TASKING",1,"init_kernel task at %#llx",task_proc);
     task_def_t *new_task = (task_def_t*)malloc(sizeof(task_def_t));
     new_task->task_id   = -1;
     new_task->task_proc = task_proc;
     new_task->arg       = arg;
     new_task->ctx = create_thread((THREAD_FUNC_T)task_proc,&new_task);

}

task_def_t *get_task(UINT64 task_id) {
     task_def_t* retval = NULL;
     retval = &(tasks[task_id]);
     return retval;
}

void kill_task(UINT64 task_id) {
     task_def_t* t = get_task(task_id);
     dmthread_t *dmthread_ctx = (dmthread_t*)t->ctx;
     dmthread_ctx->status = STATUS_DEAD;
}

void scheduler_start() {
     klog("TASKING",1,"Configuring task table");
     BS->SetMem((void*)tasks,sizeof(task_def_t)*4096,0);

}


void yield_until(EFI_EVENT e) {
     // TODO - implement a timeout of some sort
     for(;;) {
         EFI_STATUS s = BS->CheckEvent(e);
         if(s==EFI_SUCCESS) return;
         thread_yield();
     }
}

