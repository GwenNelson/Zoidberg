#ifndef VM_PAWN_H
#define VM_PAWN_H

#include "amx.h"
#include "k_thread.h"

struct pawn_vm_t {
   AMX amx;
   AMX_IDLE idlefunc;
   int is_ready;
   unsigned char* datablock;
} pawn_vm_t;

void vm_pawn_init_ctx(void **ctx, uint64_t task_id);

void vm_pawn_iterloop(void* ctx, uint64_t task_id);

void vm_pawn_cleanup(void* ctx, uint64_t task_id);

uint64_t vm_pawn_create();

void vm_pawn_exec(uint64_t task_id, char* filename);

#endif
