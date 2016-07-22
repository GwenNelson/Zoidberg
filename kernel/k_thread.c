
#include <stdio.h>
#include <stdlib.h>


#include "kmsg.h"
#include "k_thread.h"

extern EFI_BOOT_SERVICES *BS;

EFI_SIMPLETHREAD_PROTOCOL *thread_proto;

task_def_t tasks[4096];

task_def_t *pending_tasks       = NULL;
task_def_t *pending_tasks_last  = NULL;

int req_tasks_lock=0; // crappy crappy mutex

UINT64    last_task_id    = 0;
UINT64    max_task_id     = 0;
UINT64    cur_task_id     = 0;

void acquire_tasks_lock() {   // warning - this motherfucker blocks FOREVER, FOR ALL ETERNITY if already locked
     int my_id = rand();
     while(req_tasks_lock != my_id) {
        while(req_tasks_lock>0) {
          BS->Stall(120);
        }
        req_tasks_lock = my_id;
     }
}

void release_tasks_lock() {
     req_tasks_lock = 0;
}

void init_tasks() {
     acquire_tasks_lock();
     task_def_t* t = pending_tasks;
     while(t != NULL) {
       kprintf("init_tasks() found a task_req\n");
       if(t != NULL) {
          UINT64 new_task_id = init_task(t->task_proc, t->arg);
          kprintf("task_req %d honoured\n",new_task_id);
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
     if(tasks[new_task_id].task_id != 0) return;
     max_task_id++;
     kprintf("k_thread: init_task() Starting task ID %d at %#llx\n",new_task_id,task_proc);

     task_def_t new_task;

     new_task.task_id    = new_task_id;
     new_task.task_proc  = task_proc;
     new_task.arg        = arg;
     tasks[new_task_id]  = new_task;
     thread_proto->create_thread(thread_proto,(THREAD_FUNC_T)task_proc,&(tasks[new_task_id]),new_task.ctx);
     last_task_id = new_task_id; 
     return new_task.task_id;
}

task_def_t *get_task(UINT64 task_id) {
     task_def_t* retval = NULL;
     retval = &(tasks[task_id]);
     kprintf("k_thread: get_task() - located PID %d at %#llx\n",task_id,retval);
     return retval;
}

void kill_task(UINT64 task_id) {
     
}

EFI_GUID gEfiSimpleThreadProtocolGUID = EFI_SIMPLETHREAD_PROTOCOL_GUID;

void scheduler_start() {
     kprintf("k_thread: scheduler_start() Allocating task table\n");
     BS->SetMem((void*)tasks,sizeof(task_def_t)*4096,0);

     kprintf("k_thread: scheduler_start() Setting up threading protocol\n");
     EFI_STATUS s = BS->LocateProtocol(&gEfiSimpleThreadProtocolGUID , 0, (VOID**)&thread_proto);
     if(s != EFI_SUCCESS) {
        kprintf("Could not open EFI_SIMPLETHREAD_PROTOCOL, result: %d\n",s);
        return;
     }
}




