#include <efi.h>
#include <efilib.h>

#include <stdio.h>
#include <stdlib.h>

#include "kmsg.h"

extern EFI_BOOT_SERVICES *BS;
EFI_EVENT timer_ev;

typedef struct task_def_t {
    uint64_t stack_pointer;
} task_def_t;

task_def_t *tasks       = NULL;
uint64_t    task_count  = 1;
uint64_t    active_task = 0;

void timer_func(EFI_EVENT Event, void *ctx) {
     register void *esp __asm__ ("rsp");
     if(tasks==NULL) {
        tasks = malloc(sizeof(task_def_t));
        tasks[0].stack_pointer = (uint64_t)esp;
     }
     tasks[active_task].stack_pointer = (uint64_t)esp;
     active_task++;
     if(active_task >= task_count) active_task=0;
     esp = (void*)tasks[active_task].stack_pointer;
}

void thread_a() {
	int i=0;
     for(;;) {
		i++;
		kprintf("Thread A iteration %d \n",i);
	}
}

void thread_b() {
     for(;;) kprintf("Thread B\n");
}

void scheduler_start() {
     kprintf("k_thread: scheduler_start() Configuring timer\n");
     BS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)timer_func,NULL, &timer_ev);
     EFI_STATUS timer_stat = BS->SetTimer(timer_ev,TimerPeriodic,10);
     if(timer_stat != EFI_SUCCESS) {
         kprintf("k_thread: scheduler_start() Error configuring timer!\n");
     }
     thread_a();
}





