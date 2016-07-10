#include <efi.h>
#include <efilib.h>
#include <stdlib.h>

#include "kmsg.h"

extern EFI_BOOT_SERVICES *BS;

EFI_SIMPLE_NETWORK *simple_net = NULL;

void dump_net_status() {
     EFI_SIMPLE_NETWORK_MODE *m = simple_net->Mode;
     switch(m->State) {
         case EfiSimpleNetworkStopped:
           kprintf("Network status: Stopped\n");
           return;
         break;
         case EfiSimpleNetworkStarted:
           kprintf("Network status: Started\n");
           return;
         break;
         case EfiSimpleNetworkInitialized:
           kprintf("Network status: Ready\n");
         break;
         case EfiSimpleNetworkMaxState:
           kprintf("Network status: Ready\n");
         break;
     }
     if(m->MediaPresent) {
        kprintf("Network media: connected\n");
     } else {
        kprintf("Network media: not connected\n");
     }
     kprintf("MAC address: ");
     int i=0;
     for(i=0; i< m->HwAddressSize; i++) {
        kprintf("%x:",m->CurrentAddress.Addr[i]);
     }
     kprintf("\n");
}

void init_net() {
     EFI_NETWORK_INTERFACE_IDENTIFIER_INTERFACE *nii;
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
     
     kprintf("k_network: init_net() - starting networking\n");
     
     EFI_STATUS net_start_status = simple_net->Start(simple_net);
     if(net_start_status == 0) {
        kprintf("k_network: init_net() - started networking\n");
     } else {
        kprintf("k_network: init_net() - failed to start networking\n");
        kprintf("k_network: init_net() - no networking support will be available\n");
        return;
     }
     
     kprintf("k_network: init_net() - probing interfaces\n");
     handles_count = 0;
     memset((void*)handles,0,100 * sizeof(EFI_HANDLE));
     BS->LocateHandle(ByProtocol, &SimpleNetworkProtocol,NULL,&buf_size, handles);
     handles_count = buf_size == 0 ? 0 : buf_size / sizeof(EFI_HANDLE); 
     if(handles_count==0) {
        kprintf("k_network: init_net() - could not probe network interfaces\n");
        kprintf("no networking support will be available\n");
        return;
     }
     for(i=0; i < handles_count; i++) {
         EFI_STATUS nii_stat = BS->HandleProtocol(handles[i], &NetworkInterfaceIdentifierProtocol, (void**)&nii);
         if(nii_stat == 0) {
            kprintf("k_network: init_net() - setup NII on handle number %d\n",i);
         } else {
            kprintf("n_network: init_net() - failed to setup NII on handle number %d\n",i);
         }
     }

     if(nii==NULL) {
        kprintf("k_network: init_net() - no network interfaces!\n");
        kprintf("k_network: init_net() - no networking support will be available\n");
        return;
     }

     kprintf("k_network: init_net() - init network interface\n");
     EFI_STATUS init_stat = simple_net->Initialize(simple_net,0,0);

     if(init_stat==0) {
       kprintf("k_network: init_net() - network interface init ok\n");
     } else {
       kprintf("k_network: init_net() - could not init network interface\n");
       kprintf("k_network: init_net() - no networking support will be available\n");
       return;
     }

     dump_net_status();
}

