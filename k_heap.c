#include <efi.h>
#include <efilib.h>
#include <stdlib.h>

#include "efilibc.h"
#include "kmsg.h"

#include "k_heap_bitmap.c"

KHEAPBM     kheap;

int init_mem() {
    EFI_MEMORY_DESCRIPTOR  *MMap = 0;

EFI_MEMORY_DESCRIPTOR *MemoryMap         = 0;
UINTN                  MapKey            = 0;
UINTN                  DescriptorSize    = 0;
UINT32                 DescriptorVersion = 0;
    UINTN MemoryMapSize = 0;
    kprintf("k_heap:init_mem() init bitmap\n");
    k_heapBMInit(&kheap);

    kprintf("k_heap:init_mem() loading EFI memory map\n");
    EFI_STATUS stat = BS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    stat = BS->AllocatePool(EfiBootServicesData, MemoryMapSize*2, (void**)&MemoryMap);
    stat = BS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
    kprintf("k_heap:init_mem() loaded EFI memory map, dumping relevant sections and adding to bitmap:\n");
    
    int i;
    int free_boot_pages=0;
    int free_conv_pages=0;
    for(i=0; i < MemoryMapSize/(DescriptorSize); i++) {
         MMap = (EFI_MEMORY_DESCRIPTOR*)( ((CHAR8*)MemoryMap) + i * DescriptorSize);
         switch(MMap[0].Type) {
             case EfiBootServicesData:
                  free_boot_pages += MMap[0].NumberOfPages;
                  printf("BSDATA      ");
	          kprintf("%10d pages @ %#llx\n",MMap[0].NumberOfPages,MMap[0].PhysicalStart);
             break;
             case EfiConventionalMemory:
		  free_conv_pages += MMap[0].NumberOfPages;
                  printf("CONV_RAM    ");
	 	  kprintf("%10d pages @ %#llx\n",MMap[0].NumberOfPages,MMap[0].PhysicalStart);
                  k_heapBMAddBlock(&kheap,(uint64_t)MMap[0].PhysicalStart, MMap[0].NumberOfPages*EFI_PAGE_SIZE,16);
             break;
         }

     }
     kprintf("%dkb pre-heap (BSDATA) available, %dmb\n",free_boot_pages*EFI_PAGE_SIZE/1024,free_boot_pages*EFI_PAGE_SIZE/1024/1024);
     kprintf("%dkb conventional (CONV_RAM) available, %dmb\n",free_conv_pages*EFI_PAGE_SIZE/1024,free_conv_pages*EFI_PAGE_SIZE/1024/1024);

    return 0;
}

