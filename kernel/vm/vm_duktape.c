#include <stdio.h>
#include <stdlib.h>

#include "duktape.h"
#include "kmsg.h"
#include "k_thread.h"
#include "k_syscalls.h"

#include "vm_duktape.h"

extern EFI_BOOT_SERVICES *BS;

duk_ret_t duk_syscall_write(duk_context *ctx) {
     struct syscall_ctx sys_ctx;
     sys_ctx.args[0].fd    =          duk_to_uint(ctx, 0);
     sys_ctx.args[1].buf   = (void*)duk_to_string(ctx, 1);
     sys_ctx.args[2].count = (ssize_t)duk_to_uint(ctx, 2);
     sys_write(&sys_ctx);
     duk_push_number(ctx,sys_ctx.retval.ret_count);
     return 1;
}

void vm_duktape_mainproc(void* _t) {
     struct task_def_t *t = (struct task_def_t*)_t;
     struct duktape_vm_t duktape_ctx;
     char* _filename = (char*)t->arg;
     duktape_ctx.filename = _filename;
     duktape_ctx.task_id  = t->task_id;
     UINT64 task_id = t->task_id;
     kprintf("vm_duktape: exec() - task ID is %d, exec file %s\n",task_id,_filename);
     
     kprintf("vm_duktape: exec() - reading %s from disk\n",_filename);
     FILE *fd = fopen(_filename,"rb");
     if(fd == NULL) {
        kprintf("vm_duktape: exec() - failed to read!\n");
        return;
     }
     fseek(fd,0,SEEK_END);
     long fd_size = ftell(fd);
     duktape_ctx.js_src = (char*)malloc(fd_size+1);
     fseek(fd,0,SEEK_SET);
     kprintf("vm_duktape: exec() - allocated buffer of %d bytes\n",fd_size+1);
     fread(duktape_ctx.js_src,fd_size,1,fd);
     fclose(fd);
     duktape_ctx.js_src[fd_size]=0;
     duktape_ctx.ctx = duk_create_heap_default();
     if(!duktape_ctx.ctx) {
        kprintf("vm_duktape: exec() - could not create context for %d!\n",_filename);
        return;
     } else {
        kprintf("vm_duktape: exec() - created context!\n");
     }

     kprintf("vm_duktape: exec() - registering syscalls!\n");     
     duk_push_c_function(duktape_ctx.ctx, duk_syscall_write,3);
     duk_put_global_string(duktape_ctx.ctx,"write");
     kprintf("vm_duktape: exec() - loading bytecode into context!\n");

     duk_push_lstring(duktape_ctx.ctx, duktape_ctx.js_src, fd_size);
     duk_to_buffer(duktape_ctx.ctx, -1, NULL);
     kprintf("vm_duktape: exec() - preparing for execution!\n");
     duk_load_function(duktape_ctx.ctx);
     duk_push_global_object(duktape_ctx.ctx);
     kprintf("vm_duktape: exec() - process ready!\n");
     duk_call(duktape_ctx.ctx,0);
     kprintf("%s\n",duk_get_string(duktape_ctx.ctx,-1));
     free(duktape_ctx.js_src);
     duk_destroy_heap(duktape_ctx.ctx);
}
