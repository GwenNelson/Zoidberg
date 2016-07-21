#ifndef VM_DUKTAPE_H
#define VM_DUKTAPE_H

#include "duktape.h"
#include "k_thread.h"

struct duktape_vm_t {
   UINT64 task_id;
   char* filename;
   char* js_src;
   duk_context *ctx;
};

void vm_duktape_mainproc(void* _t);

#endif
