#include <stdio.h>
#include <stdlib.h>

#include "amx.h"
#include "kmsg.h"
#include "k_thread.h"

#include "vm_pawn.h"

void vm_pawn_mainproc(void* filename, UINT64 task_id) {
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
     amx_Align16((uint16_t *)&hdr.flags);
     amx_Align32((uint32_t *)&hdr.size);
     amx_Align32((uint32_t *)&hdr.cod);
     amx_Align32((uint32_t *)&hdr.dat);
     amx_Align32((uint32_t *)&hdr.hea);
     amx_Align32((uint32_t *)&hdr.stp);
     kprintf("AMX magic is %x\n",hdr.magic);
     kprintf("vm_pawn: exec() - Setting up AMX VM with %d bytes\n",hdr.size);
/*
     memset((void*)&(pawn_ctx->amx),0, sizeof(AMX));
     kprintf("memset\n");
     pawn_ctx->datablock = (unsigned char*)malloc(hdr.size);
     kprintf("malloc\n");
     zoidberg_fread(pawn_ctx->datablock, 1, hdr.size-hdr.hea, fd);
     kprintf("fread\n");
     zoidberg_fclose(fd);
     int result=amx_Init(&(pawn_ctx->amx), pawn_ctx->datablock);
     if(result != AMX_ERR_NONE) {
        kprintf("vm_pawn: exec() - Could not load AMX image\n");
        free(pawn_ctx->datablock);
        pawn_ctx->amx.base = NULL;
        return;
     }
     kprintf("vm_pawn: exec() - Loaded AMX image into task\n");
     pawn_ctx->is_ready = 1;*/
}
