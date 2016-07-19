
#include <stdio.h>
#include <stdlib.h>


#include "kmsg.h"
#include "k_thread.h"

extern EFI_BOOT_SERVICES *BS;

EFI_SIMPLETHREAD_PROTOCOL *thread_proto;

struct task_def_t tasks[4096];
UINT64    last_task_id    = 0;
UINT64    max_task_id     = 0;
UINT64    cur_task_id     = 0;

UINT64 init_task(void (*task_proc)(void* arg, UINT64 task_id), void* arg) {
     UINT64 new_task_id = last_task_id+1;
     max_task_id++;
     kprintf("k_thread: init_task() Starting task ID %d at %#llx\n",new_task_id,task_proc);

     struct task_def_t new_task;

     new_task.task_id    = new_task_id;
     new_task.task_proc  = task_proc;
     new_task.arg        = arg;
     thread_proto->create_thread(thread_proto,(THREAD_FUNC_T)task_proc,arg,new_task.ctx);
     
     return new_task.task_id;
}

struct task_def_t *get_task(UINT64 task_id) {
     struct task_def_t* retval = NULL;
     retval = &(tasks[task_id]);
     kprintf("k_thread: get_task() - located PID %d at %#llx\n",task_id,retval);
     return retval;
}

void kill_task(UINT64 task_id) {
     
}

EFI_GUID gEfiSimpleThreadProtocolGUID = EFI_SIMPLETHREAD_PROTOCOL_GUID;

void scheduler_start() {
     kprintf("k_thread: scheduler_start() Allocating task table\n");
     BS->SetMem((void*)tasks,sizeof(struct task_def_t)*4096,0);

     kprintf("k_thread: scheduler_start() Setting up threading protocol\n");
     EFI_STATUS s = BS->LocateProtocol(&gEfiSimpleThreadProtocolGUID , 0, (VOID**)&thread_proto);
     if(s != EFI_SUCCESS) {
        kprintf("Could not open EFI_SIMPLETHREAD_PROTOCOL, result: %d\n",s);
        return;
     }
}




