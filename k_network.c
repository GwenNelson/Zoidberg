#include <efi.h>
#include <efilib.h>
#include <stdlib.h>

#include <netinet/udp.h>
#include <netinet/ip.h>

#include "kmsg.h"

extern EFI_BOOT_SERVICES *BS;
extern EFI_GUID Udp4Protocol;
extern EFI_HANDLE gImageHandle;
extern EFI_LOADED_IMAGE *g_li;
EFI_HANDLE netHandle;

EFI_SIMPLE_NETWORK *simple_net = NULL;
EFI_PXE_BASE_CODE  *pxe_base   = NULL;

#define htonl __htonl

unsigned short csum(unsigned short *ptr,int nbytes) 
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
 
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }
 
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
     
    return(answer);
}

void configure_net_dhcp() {
     kprintf("k_network: configure_net_dhcp() - attempting DHCP configuration\n");

     kprintf("k_network: configure_net_dhcp() - building DHCP request\n");
     EFI_SIMPLE_NETWORK_MODE *m = simple_net->Mode;

     void* buf=malloc(4096);
     memset(buf,0,4096);

     UINTN pack_size = sizeof(EFI_PXE_BASE_CODE_DHCPV4_PACKET);
//     EFI_PXE_BASE_CODE_DHCPV4_PACKET *dhcp_req = (EFI_PXE_BASE_CODE_DHCPV4_PACKET*)malloc(sizeof(EFI_PXE_BASE_CODE_DHCPV4_PACKET));
     EFI_PXE_BASE_CODE_DHCPV4_PACKET *dhcp_req = (EFI_PXE_BASE_CODE_DHCPV4_PACKET*)(buf + (sizeof(struct iphdr) + sizeof(struct udphdr)));

     uint32_t req_id = (uint32_t)rand();

     int i=0;

     memset((void*)dhcp_req,0,pack_size);
     dhcp_req->BootpOpcode    = 1; // request
     dhcp_req->BootpHwType    = m->IfType;
     dhcp_req->BootpHwAddrLen = m->HwAddressSize;
     dhcp_req->BootpGateHops  = 0;
     dhcp_req->BootpIdent     = req_id;
     dhcp_req->BootpSeconds   = 0;
     dhcp_req->BootpFlags     = 0x8000;
     for(i=0; i< m->HwAddressSize; i++) {
         dhcp_req->BootpHwAddr[i] = m->CurrentAddress.Addr[i];
     }
     dhcp_req->DhcpMagik = 0x63825363;

     dhcp_req->DhcpOptions[0] = 53;
     dhcp_req->DhcpOptions[1] = 1;
     dhcp_req->DhcpOptions[2] = 1;

     struct iphdr *iph = (struct iphdr *)buf;
     struct udphdr *udph = (struct udphdr *) (buf + sizeof (struct iphdr));
     void* data = buf + (sizeof(struct iphdr) + sizeof(struct udphdr));
     iph->ihl      = 5;
     iph->version  = 4;
     iph->tot_len  = sizeof(struct iphdr) + sizeof(struct udphdr) + pack_size;
     iph->id       = htonl(req_id+1);
     iph->frag_off = 0;
     iph->ttl      = 255;
     iph->protocol = IPPROTO_UDP;
     iph->saddr    = INADDR_ANY;
     iph->daddr    = INADDR_BROADCAST;
     iph->check    = csum ((unsigned short *) buf, iph->tot_len);
     udph->uh_ulen = sizeof(struct udphdr) + pack_size;
     udph->uh_sum  = csum((unsigned short*)(data - sizeof(struct udphdr)),pack_size+sizeof(struct udphdr));

     EFI_MAC_ADDRESS anywhere;
     for(i=0; i< m->HwAddressSize; i++) {
         anywhere.Addr[i] = 255;
     }
     

     kprintf("k_network:configure_net_dhcp() - Transmitting request\n");
     UINTN tx_size=pack_size+(sizeof(struct iphdr))+(sizeof(struct udphdr));
     UINT16 ether_type = 0x8000;
//     EFI_STATUS send_s = simple_net->Transmit(simple_net,0,pack_size+(sizeof(struct iphdr))+(sizeof(struct udphdr)),buf,NULL,NULL,NULL);
     EFI_STATUS send_s = simple_net->Transmit(simple_net,simple_net->Mode->MediaHeaderSize,tx_size,buf,NULL,&anywhere,&ether_type);

     kprintf("k_network:configure_net_dhcp() - Waiting for transmission\n");
     void* tx_buf=NULL;
     while(tx_buf==NULL && (send_s==0)) {
       tx_buf=NULL;
       send_s = simple_net->GetStatus(simple_net,0,&tx_buf);
       if(send_s != 0) kprintf("!\n");
     }
     kprintf("k_network:configure_net_dhcp() - Transmitted, tx_buf at  %#llx \n",tx_buf);
}



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

     EFI_NETWORK_STATISTICS *stats;
     stats = (void*)malloc(sizeof(EFI_NETWORK_STATISTICS));
     UINTN stats_size=0;
     simple_net->Statistics(simple_net,FALSE,&stats_size,stats);
     free(stats);
     stats = (void*)malloc(stats_size);
     simple_net->Statistics(simple_net,FALSE,&stats_size,stats);

     kprintf("Transmitted %d bytes\n",stats->TxTotalBytes);
     kprintf("CRC errors: %d\n",stats->TxCrcErrorFrames);
     free(stats);
}

void configure_net_pxe_basecode() {
     EFI_STATUS s = BS->OpenProtocol(g_li->DeviceHandle, &PxeBaseCodeProtocol, (void**)&pxe_base, gImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
     kprintf("Status %d\n",s);
     kprintf("Found PXE base code at %#llx\n",pxe_base);
}

void init_net() {
     EFI_NETWORK_INTERFACE_IDENTIFIER_INTERFACE *nii;
     EFI_HANDLE handles[100];
     int handles_count;

     UINTN buf_size = sizeof(EFI_HANDLE);

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
            netHandle = handles[i];
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
            netHandle = handles[i];
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
     EFI_STATUS init_stat = simple_net->Initialize(simple_net,4096,4096);

     if(init_stat==0) {
       kprintf("k_network: init_net() - network interface init ok\n");
     } else {
       kprintf("k_network: init_net() - could not init network interface\n");
       kprintf("k_network: init_net() - no networking support will be available\n");
       return;
     }

     configure_net_pxe_basecode();

     configure_net_dhcp();
     dump_net_status();
}

