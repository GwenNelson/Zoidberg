#include <efi.h>
#include <efilib.h>
#include <efilibc.h>
#include <stdlib.h>
#include <stdio.h>
#include <physfs.h>

EFI_SYSTEM_TABLE *ST;
EFI_BOOT_SERVICES *BS;
EFI_RUNTIME_SERVICES *RT;
EFI_HANDLE gImageHandle;

int gStartupFreePages;

EFI_EVENT timer_ev;

void init_net() {
    EFI_NETWORK_INTERFACE_IDENTIFIER_INTERFACE *nii;
    EFI_SIMPLE_NETWORK *simple_net;   
    EFI_HANDLE handles[100];
    UINTN buf_size = 100 * sizeof(EFI_HANDLE); 
 
    printf("Locating SNP handle...\n");
    BS->LocateHandle(ByProtocol, &SimpleNetworkProtocol,NULL,&buf_size, handles);
    int handles_count;
    handles_count = buf_size == 0 ? 0 : buf_size / sizeof(EFI_HANDLE);

    printf("Located %d handles\n",handles_count);

  int i;
    for(i=0; i < handles_count; i++) {
      EFI_STATUS prot_stat = BS->HandleProtocol(handles[i], &SimpleNetworkProtocol, (void**)&simple_net);
    }

    if(simple_net==NULL) {
       printf("Could not open EFI_SIMPLE_NETWORK!\n");
    } else {

         printf("Starting up networking:\n");
         EFI_STATUS net_start_status = simple_net->Start(simple_net);

         if(net_start_status != EFI_SUCCESS) {
            printf("Error starting networking!\n");
         } else {
            printf("Networking started!\n");
         }
    

         BS->HandleProtocol(gImageHandle, &NetworkInterfaceIdentifierProtocol, (void**)&nii);
         if(nii->ID == 0) {
            printf("No network interface found!\n");
         }
    }
}

void init_ram() {
     gStartupFreePages = 0;
     UINTN                  MemoryMapSize     = 0;
     EFI_MEMORY_DESCRIPTOR *MemoryMap         = 0;
     UINTN                  MapKey            = 0;
     UINTN                  DescriptorSize    = 0;
     UINT32                 DescriptorVersion = 0;
     EFI_MEMORY_DESCRIPTOR  *MMap             = 0;
     EFI_STATUS stat = BS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
     stat = BS->AllocatePool(EfiBootServicesData, MemoryMapSize, (void**)&MemoryMap);
     stat = BS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
     int i;
     for(i=0; i < MemoryMapSize/(DescriptorSize); i++) {
         MMap = (EFI_MEMORY_DESCRIPTOR*)( ((CHAR8*)MemoryMap) + i * DescriptorSize);
         switch(MMap[0].Type) {
             case EfiReservedMemoryType:
                  printf("RESERVED    ");
             break;
             case EfiLoaderCode:
                  printf("LOADERCODE  ");
             break;
             case EfiLoaderData:
                  printf("LOADERDATA  ");
             break;
             case EfiBootServicesCode:
                  printf("BS_CODE     ");
             break;
             case EfiBootServicesData:
                  printf("BS_DATA     ");
             break;
             case EfiRuntimeServicesCode:
                  printf("RS_CODE     ");
             break;
             case EfiRuntimeServicesData:
                  printf("RS_DATA     ");
             break;
             case EfiConventionalMemory:
                  gStartupFreePages += MMap[0].NumberOfPages;
                  printf("CONV_RAM    ");
             break;
             case EfiUnusableMemory:
                  printf("UNUSABLE    ");
             break;
             case EfiACPIReclaimMemory:
                  printf("ACPIRECLAIM ");
             break;
             case EfiACPIMemoryNVS:
                  printf("ACPI NVS    ");
             break;
             case EfiMemoryMappedIO:
                  printf("MMIO        ");
             break;
             case EfiMemoryMappedIOPortSpace:
                  printf("MMIO_PORTSP ");
             break;
             case EfiPalCode:
                  printf("PAL_CODE    ");
             break;
             case EfiMaxMemoryType:
                  printf("MAX_MEM     ");
             break;
         }
         printf("%10d pages @ %#llx\n",MMap[0].NumberOfPages,MMap[0].PhysicalStart);
     }
     printf("Have %d free pages\n",gStartupFreePages);
     printf("%d kb available, %d mb\n",gStartupFreePages*EFI_PAGE_SIZE, (gStartupFreePages*EFI_PAGE_SIZE)/1024/1024 );
}

void handle_syscall() {
     __asm__("cli; hlt"); 
}

void timer_func(EFI_EVENT Event, void *ctx) {
}

void dump_vfs(const char* path);
void dump_vfs(const char* path) {
     char **rc;
     char **i;

     rc = PHYSFS_enumerateFiles(path);
     char buf[1024];
     for(i=rc; *i !=NULL; i++) {
         sprintf(buf,"%s/%s",path,*i);
         printf("path is %s\n",path);
         printf("*i is %s\n",*i);
         printf("buf is %s\n",buf);
         if(PHYSFS_isDirectory(buf)) {
           printf("dump_vfs %s\n",buf);
           dump_vfs(buf);
         } else {
           printf("%s\n",buf,*i);
         }
     }
     PHYSFS_freeList(rc);


}
 
EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    ST = SystemTable;
    BS = ST->BootServices;
    RT = ST->RuntimeServices;
    gImageHandle = ImageHandle;
    efilibc_init(ImageHandle);
    
    EFI_LOADED_IMAGE *li;
 
    BS->HandleProtocol(ImageHandle, &LoadedImageProtocol, (void**)&li);
    printf("Kernel loaded at: %#llx\n", (uint64_t)li->ImageBase);
    printf("Kernel entry point (efi_main) located at: %#11x\n", (uint64_t)efi_main);
    init_ram();

    printf("Disabling UEFI Watchdog\n");
    BS->SetWatchdogTimer(0, 0, 0, NULL);

    printf("Setting up scheduler\n");
    BS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (EFI_EVENT_NOTIFY)timer_func,NULL, &timer_ev);
    EFI_STATUS timer_stat = BS->SetTimer(timer_ev,TimerPeriodic,0);
    if(timer_stat != EFI_SUCCESS) {
       printf("Error configuring timer!\n");
    }

    printf("Setting up VFS\n");

    PHYSFS_init(NULL);


    printf("Mounting EFI filesystem\n");   
    int retval=0;
    retval = PHYSFS_mount("/","/efi",0);
    
    printf("Dumping VFS:\n");
    dump_vfs("/");
 
    printf("Ready to do stuff\n");

    while(1) {
    } 
}
