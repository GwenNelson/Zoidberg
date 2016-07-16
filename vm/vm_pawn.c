#include <efi.h>
#include <efilib.h>

#include <stdio.h>
#include <stdlib.h>

#include "amx.h"
#include "kmsg.h"
#include "k_thread.h"

struct pawn_vm_t {
   AMX amx;
   AMX_IDLE idlefunc;
   int is_ready;
} pawn_vm_t;

void vm_pawn_init_ctx(void *ctx, uint64_t task_id) {
     ctx = malloc(sizeof(struct pawn_vm_t));
     struct pawn_vm_t *pawn_ctx = (struct pawn_vm_t*)ctx;
     pawn_ctx->is_ready = 0;
}

void vm_pawn_iterloop(void* ctx, uint64_t task_id) {
     struct pawn_vm_t *pawn_ctx = (struct pawn_vm_t*)ctx;
     if(pawn_ctx->is_ready == 0) {
        if(amx_GetUserData(&(pawn_ctx->amx), AMX_USERTAG('I','d','l','e'), (void**)&(pawn_ctx->idlefunc)) != AMX_ERR_NONE) {
           pawn_ctx->idlefunc = NULL;
        }
     }
}

void vm_pawn_cleanup(void* ctx, uint64_t task_id) {
}

uint64_t vm_pawn_create() {
     uint64_t new_task = init_task(&vm_pawn_init_ctx, &vm_pawn_cleanup, &vm_pawn_iterloop);
     return new_task;
}

void vm_pawn_exec(uint64_t task_id, char* filename) {
     kprintf("vm_pawn: exec() - task ID is %d, exec file %s\n",task_id,filename);
     struct task_def_t *t       = get_task(task_id);
     struct pawn_vm_t *pawn_ctx = (struct pawn_vm_t*)t->ctx;
     if(pawn_ctx->is_ready == 1) { // TODO - do stuff here to reset it after a fork
     }
     
}
