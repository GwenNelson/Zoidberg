#include <stdio.h>
#include <stdlib.h>

#include "amx.h"
#include "amxaux.h"
#include "kmsg.h"
#include "k_thread.h"

#include "vm_pawn.h"

extern EFI_BOOT_SERVICES *BS;

int AMXAPI vm_pawn_monitor(AMX *amx) {
    kprintf("vm_pawn_monitor called!\n");
    return AMX_ERR_NONE;
}

void vm_pawn_mainproc(void* filename, UINT64 task_id) {
     struct pawn_vm_t pawn_ctx;
     char* _filename = (char*)filename;
     kprintf("vm_pawn: exec() - task ID is %d, exec file %s\n",task_id,_filename);

     struct task_def_t *t       = get_task(task_id);
     kprintf("vm_pawn: exec() - task struct at %#llx\n", t);
     kprintf("vm_pawn: exec() - reading AMX header\n");
     FILE* fd = fopen(filename,"rb");
     if(fd==NULL) {
        kprintf("vm_pawn: exec() - Could not open %s\n",_filename);
        return;
     }

     AMX_HEADER hdr;
     fread(&hdr, 1, sizeof(AMX_HEADER), fd);
     kprintf("vm_pawn: exec() - parsing AMX header\n");
     amx_Align16(&hdr.magic);
     amx_Align16((UINT16 *)&hdr.flags);
     amx_Align32((UINT32 *)&hdr.size);
     amx_Align32((UINT32 *)&hdr.cod);
     amx_Align32((UINT32 *)&hdr.dat);
     amx_Align32((UINT32 *)&hdr.hea);
     amx_Align32((UINT32 *)&hdr.stp);
     kprintf("AMX magic is %x\n",hdr.magic);
     kprintf("vm_pawn: exec() - Setting up AMX VM with %d bytes\n",hdr.size);

     fclose(fd);
     BS->SetMem((void*)&(pawn_ctx.amx),sizeof(AMX),0);
     int result = aux_LoadProgram(&(pawn_ctx.amx),_filename,NULL);
     if(result != AMX_ERR_NONE) {
        kprintf("vm_pawn: exec() - Could not load AMX image: %s\n",aux_StrError(result));
        aux_FreeProgram(&(pawn_ctx.amx));
        return;
     }
     kprintf("vm_pawn: exec() - Loaded AMX image into task\n");
     kprintf("vm_pawn: exec() - Starting VM\n");     

     amx_SetDebugHook(&(pawn_ctx.amx),vm_pawn_monitor);
     cell ret = 0;
     result   = amx_Exec(&(pawn_ctx.amx), &ret, AMX_EXEC_MAIN);
     while(result == AMX_ERR_SLEEP) {
        BS->Stall(pawn_ctx.amx.pri);
        result = amx_Exec(&(pawn_ctx.amx), &ret, AMX_EXEC_CONT);
     }
     if(result != AMX_ERR_NONE) {
        kprintf("vm_pawn: %s\n", aux_StrError(result));
     }
     if(ret != 0) {
        kprintf("vm_pawn: Task %d returned %ld\n", task_id,(long)ret);
     }
     aux_FreeProgram(&(pawn_ctx.amx));
}
