#ifndef VM_PAWN_H
#define VM_PAWN_H

#include "amx.h"
#include "k_thread.h"

struct pawn_vm_t {
   AMX amx;
   AMX_IDLE idlefunc;
   unsigned char* datablock;
};

void vm_pawn_mainproc(void* filename, UINT64 task_id);

#endif
