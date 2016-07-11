#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>

#include "kmsg.h"
#include "cr.c"

jmp_buf main_task;
jmp_buf child_task; 

void thread_a();
void thread_b();

int thread_count=3;
int i=0;

void scheduler_thread() {
     CR_THREAD_INIT();
     while(1) {
        for(i=1; i<= thread_count; i++) {
            cr_g_activate_id = i;
            CR_YIELD(cr_idle);
        }
     }
}

void thread_a() {
     CR_THREAD_INIT();
     for(;;) {
        kprintf("thread A! \n");
        
        CR_YIELD(scheduler_thread);
     }
}

void thread_b() {
     CR_THREAD_INIT();
     for(;;) {
        kprintf("thread B!\n");
        CR_YIELD(scheduler_thread);
     }
}

void scheduler_start() {
     kprintf("k_thread.c: scheduler_start setting up thread contexts\n");
     CR_CONTEXT context_array[4];
     cr_init(context_array,4);
     kprintf("k_thread.c: scheduler_start creating scheduler thread\n");
     cr_register_thread(cr_idle);
     cr_register_thread(scheduler_thread);


     kprintf("k_thread.c: scheduler_start creating test threads A+B\n");
     cr_register_thread(thread_a);
     cr_register_thread(thread_b);
     kprintf("k_thread.c: scheduler_start switching to threads\n");
     CR_START(scheduler_thread);
     while(1) {}
}

