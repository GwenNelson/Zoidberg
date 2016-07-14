#include <efi.h>
#include <efilib.h>
#include <stdlib.h>

#include <netinet/udp.h>
#include <netinet/ip.h>

#include "kmsg.h"

extern EFI_BOOT_SERVICES *BS;
EFI_HANDLE netHandle;

EFI_SIMPLE_NETWORK *simple_net = NULL;

extern char *kmsg;

#define htonl __htonl
#define htons __htons

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

     char _tx_buf[4096];
     void* buf = (void*)_tx_buf;

     memset(buf,0,4096);
//     UINTN pack_size = sizeof(struct dhcp_msg);
     UINTN pack_size=0;
//     EFI_PXE_BASE_CODE_DHCPV4_PACKET *dhcp_req = (EFI_PXE_BASE_CODE_DHCPV4_PACKET*)malloc(sizeof(EFI_PXE_BASE_CODE_DHCPV4_PACKET));
     struct dhcp_msg *dhcp_req = (struct dhcp_msg*)(buf +(m->MediaHeaderSize)+ (sizeof(struct iphdr) + sizeof(struct udphdr)));

     uint64_t req_id = (uint64_t)rand();

     int i=0;

/*     memset((void*)dhcp_req,0,pack_size);
     dhcp_req->op    = 1; // request
     dhcp_req->htype    = m->IfType;
     dhcp_req->hlen = m->HwAddressSize;
     dhcp_req->hops  = 0;
     dhcp_req->xid     = req_id;
     dhcp_req->secs   = 0;
     dhcp_req->flags     = 0x8000;
     for(i=0; i< m->HwAddressSize; i++) {
         dhcp_req->chaddr[i] = m->CurrentAddress.Addr[i];
     }
     dhcp_req->cookie = 0x63825363;

     dhcp_req->options[0] = 53;
     dhcp_req->options[1] = 1;
     dhcp_req->options[2] = 1;*/


     kprintf("k_network: configure_net_dhcp() ID is %#llx\n",req_id);
     struct iphdr *iph = (struct iphdr *)(buf+(m->MediaHeaderSize));
     struct udphdr *udph = (struct udphdr *) (buf + sizeof (struct iphdr)+(m->MediaHeaderSize));
     memset((void*)iph,0,sizeof(struct iphdr));
//     memset((void*)udph,0,sizeof(struct udph));
     iph->ihl = 5;
     iph->version = 4;
     iph->tot_len  = htons(sizeof(struct iphdr));
     iph->protocol = IPPROTO_UDP;
     iph->ttl      = 255;
     iph->saddr    = INADDR_ANY;
     iph->daddr    = INADDR_ANY;
//     iph->check    = csum ((unsigned short *) buf, iph->tot_len);
//     udph->uh_ulen = sizeof(struct udphdr) + pack_size;
//     udph->uh_sum  = csum((unsigned short*)(buf - sizeof(struct udphdr)),pack_size+sizeof(struct udphdr));
//     udph->uh_sport  = 68;
//     udph->uh_dport    = 68;

     EFI_MAC_ADDRESS anywhere;
     for(i=0; i< m->HwAddressSize; i++) {
         anywhere.Addr[i] = 255;
     }
     
     char* char_iph = (char*)iph;
     char_iph[0]='H';
     char_iph[1]='i';
     kprintf("k_network:configure_net_dhcp() - Transmitting request\n");
     UINTN tx_size=(sizeof(struct iphdr));
     UINT16 ether_type = 0x800;
//     EFI_STATUS send_s = simple_net->Transmit(simple_net,0,tx_size,buf,NULL,&anywhere,&ether_type);
     EFI_STATUS send_s = simple_net->Transmit(simple_net,m->MediaHeaderSize,tx_size,(void*)buf,NULL,&anywhere,&ether_type);

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

     kprintf("Transmitted %llu bytes\n",stats->TxTotalBytes);
     kprintf("CRC errors: %llu\n",stats->TxCrcErrorFrames);
     free(stats);
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


     simple_net->Statistics(simple_net,TRUE,NULL,NULL);
     configure_net_dhcp();
     dump_net_status();
}

