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

void scheduler_thread() {
     CR_THREAD_INIT();
     int i=0;
     for(i=1; i<= thread_count; i++) {
         cr_g_activate_id = i;
         CR_YIELD(cr_idle);
     }
}

void thread_a() {
     CR_THREAD_INIT();
     for(;;) {
        printf("thread A! %d %d\n",cr_get_id(thread_a),cr_get_id(thread_b));
        
        CR_YIELD(scheduler_thread);
     }
}

void thread_b() {
     CR_THREAD_INIT();
     for(;;) {
        printf("thread B!\n");
        CR_YIELD(scheduler_thread);
     }
}

void scheduler_start() {
     CR_CONTEXT context_array[4];
     cr_init(context_array,4);
     cr_register_thread(scheduler_thread);
     cr_register_thread(thread_a);
     cr_register_thread(thread_b);
     CR_START(thread_a);
}

