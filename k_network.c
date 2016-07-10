#include <efi.h>
#include <efilib.h>
#include <stdlib.h>

#include "kmsg.h"

extern EFI_BOOT_SERVICES *BS;

void init_net() {
     EFI_NETWORK_INTERFACE_IDENTIFIER_INTERFACE *nii;
     EFI_SIMPLE_NETWORK *simple_net=NULL;   
     EFI_HANDLE handles[100];
     int handles_count;

     UINTN buf_size = 100 * sizeof(EFI_HANDLE);

     kprintf("k_network: init_net() - probing firmware for networking support\n");
     BS->LocateHandle(ByProtocol, &SimpleNetworkProtocol,NULL,&buf_size, handles);
     handles_count = buf_size == 0 ? 0 : buf_size / sizeof(EFI_HANDLE);
     
     if(handles_count==0) {
        kprintf("k_network: init_net() - no SNP handles found, does firmware have drivers for your NIC?\n");
        kprintf("k_network: init_net() - no networking support will be available\n");
        return;
     }

     kprintf("k_network: init_net() - located %d SNP handles\n", handles_count);
     int i;
     for(i=0; i < handles_count; i++) {
         EFI_STATUS prot_stat = BS->HandleProtocol(handles[i], &SimpleNetworkProtocol, (void**)&simple_net);
         if(prot_stat == 0) {
            kprintf("k_network: init_net() - setup SNP on handle number %d\n", i);
         } else {
            kprintf("k_network: init_net() - failed to setup SNP on handle number %d\n", i);
         }
     }

     if(simple_net == NULL) {
        kprintf("k_network: init_net() - failed to setup any SNP handles\n");
        kprintf("k_network: init_net() - no networking support will be available\n");
        return;
     }
}

