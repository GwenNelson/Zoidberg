#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <sys/EfiSysCall.h>
#include <Library/UefiLib.h>

char *kmsg=NULL;

static char static_kmsg[4096];
static int is_static=1;
static int buf_len=4096;

volatile UINT8 kmsg_lock=0;
volatile UINT8 klog_lock=0;

void acquire_kmsg_lock() {
     while(__sync_lock_test_and_set(&kmsg_lock, 1)) {
     }
}

void acquire_klog_lock() {
     while(__sync_lock_test_and_set(&klog_lock, 1)) {
     }
}

void release_kmsg_lock() {
     __sync_synchronize();
     kmsg_lock=0;
}

void release_klog_lock() {
     __sync_synchronize();
     klog_lock=0;
}

extern EFI_SYSTEM_TABLE *ST;

void init_static_kmsg() {
     memset((void*)static_kmsg,0,4096);
}

int kprintf(const char *fmt, ...);
void init_dynamic_kmsg() {
	kmsg = realloc((void*)kmsg, 4096);
	memset((void*)kmsg, 0, 4096);
	strcat(kmsg,static_kmsg);
	is_static = 0;
}

int kprintf(const char *fmt, ...)
{
	acquire_kmsg_lock();
	char temp_buf[4096];
	memset((void*)temp_buf, 0, 4096);
	va_list ap;
	va_start(ap, fmt);
	int retval = vsprintf(temp_buf,fmt,ap);
	va_end(ap);
	if(is_static==0) {
		if((strlen(temp_buf)+strlen(kmsg))>buf_len) {
			kmsg = realloc((void*)kmsg, strlen(kmsg)+strlen(temp_buf)+8192); // always tag on a few kb so we don't need to reallocate every time
			buf_len = strlen(kmsg)+strlen(temp_buf)+8192;
		}
		strcat(kmsg,temp_buf);
	} else {
		strcat(static_kmsg,temp_buf);
	}
	printf(temp_buf);
	release_kmsg_lock();
        return retval;
}

int kvprintf(const char *fmt, va_list ap)
{
	acquire_kmsg_lock();
	char temp_buf[4096];
	memset((void*)temp_buf, 0, 4096);
	int retval = vsprintf(temp_buf,fmt,ap);
	if(is_static==0) {
		if((strlen(temp_buf)+strlen(kmsg))>buf_len) {
			kmsg = realloc((void*)kmsg, strlen(kmsg)+strlen(temp_buf)+8192); // always tag on a few kb so we don't need to reallocate every time
			buf_len = strlen(kmsg)+strlen(temp_buf)+8192;
		}
		strcat(kmsg,temp_buf);
	} else {
		strcat(static_kmsg,temp_buf);
	}
	printf(temp_buf);
	release_kmsg_lock();
        return retval;
}


int klog(char* component, int is_good, const char *fmt, ...) {
    acquire_klog_lock();
    if(is_good==1) {
       ST->ConOut->SetAttribute(ST->ConOut,EFI_TEXT_ATTR(EFI_GREEN|0x8,EFI_BACKGROUND_BLACK));
    } else {
       ST->ConOut->SetAttribute(ST->ConOut,EFI_TEXT_ATTR(EFI_RED|0x8,EFI_BACKGROUND_BLACK));
    }
    char temp_buf[15];
    char comp_buf[15];
    snprintf(comp_buf,15,"[%s]",component);
    snprintf(temp_buf,15,"%-10s ",comp_buf);
    int i;
    CHAR16 str[] = {0,0};
    for(i=0; i<strlen(temp_buf); i++) {
       str[0] = (CHAR16)(temp_buf[i]);
       ST->ConOut->OutputString(ST->ConOut,str);
    }
    ST->ConOut->SetAttribute(ST->ConOut,EFI_TEXT_ATTR(EFI_LIGHTGRAY,EFI_BACKGROUND_BLACK));
    va_list ap;
    va_start(ap, fmt);
    int retval = kvprintf(fmt,ap);
    va_end(ap);
    kprintf("\n");
    release_klog_lock();
} 
