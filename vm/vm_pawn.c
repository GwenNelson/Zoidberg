#include <efi.h>
#include <efilib.h>

#include <stdio.h>
#include <stdlib.h>

#include "amx.h"
#include "kmsg.h"
#include "k_thread.h"

#include "vm_pawn.h"

typedef struct ZOIDBERG_FILE
{
       EFI_FILE *f;
       int eof;
       int error;
       int fileno;
       int istty;
       int ttyno;
} ZOIDBERG_FILE;

void vm_pawn_init_ctx(void **ctx, uint64_t task_id) {
     kprintf("vm_pawn: vm_pawn_init_ctx() setting up ctx\n");
     struct pawn_vm_t *pawn_ctx = (struct pawn_vm_t*)*ctx;
     pawn_ctx = malloc(sizeof(struct pawn_vm_t));
     pawn_ctx->is_ready = 0;
     *ctx = pawn_ctx;
     kprintf("vm_pawn: vm_pawn_init_ctx() done\n");
}

void vm_pawn_iterloop(void* ctx, uint64_t task_id) {
     struct pawn_vm_t *pawn_ctx = (struct pawn_vm_t*)ctx;
     if(pawn_ctx->is_ready == 0) {
        kprintf("vm_pawn: vm_pawn_iterloop() - task %d not ready to execute yet\n",task_id);
        return;
     } 
}

void vm_pawn_cleanup(void* ctx, uint64_t task_id) {
}

uint64_t vm_pawn_create() {
     uint64_t new_task = init_task(&vm_pawn_init_ctx, &vm_pawn_cleanup, &vm_pawn_iterloop);
     return new_task;
}


ZOIDBERG_FILE *zoidberg_fopen(const char *path, const char *mode);
size_t zoidberg_fread(void *ptr, size_t size, size_t nmemb, ZOIDBERG_FILE *stream);
void vm_pawn_exec(uint64_t task_id, char* filename) {
     kprintf("vm_pawn: exec() - task ID is %d, exec file %s\n",task_id,filename);

     struct task_def_t *t       = get_task(task_id);
     kprintf("vm_pawn: exec() - task struct at %#llx\n", t);
     struct pawn_vm_t *pawn_ctx = (struct pawn_vm_t*)t->ctx;
     kprintf("vm_pawn: exec() - pawn context at %#llx\n",t->ctx);
     if(pawn_ctx->is_ready == 1) { // TODO - do stuff here to reset it after a fork
     }
     kprintf("vm_pawn: exec() - reading AMX header\n");
     ZOIDBERG_FILE* fd = zoidberg_fopen(filename,"rb");
     if(fd==NULL) {
        kprintf("vm_pawn: exec() - Could not open %s\n",filename);
        return;
     }

     AMX_HEADER hdr;
     zoidberg_fread(&hdr, 1, sizeof(AMX_HEADER), fd);
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
     pawn_ctx->is_ready = 1;
}
