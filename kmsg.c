#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

char *kmsg=NULL;

static char static_kmsg[4096];
static int is_static=1;

#define MAXBUF 32768

void init_static_kmsg() {
     memset((void*)static_kmsg,0,4096);
}

int kprintf(const char *fmt, ...);
void init_dynamic_kmsg() {
	kprintf("kmsg: about to switch to dynamic buffer, if things go silent after this we have memory issues\n");
	kmsg = realloc((void*)kmsg, strlen(4096));
	memset((void*)kmsg, 0, 4096);
	strcat(kmsg,static_kmsg);
	kprintf("kmsg: allocated dynamic buffer!\n");
}

int kprintf(const char *fmt, ...)
{
	char temp_buf[4096];
	memset((void*)temp_buf, 0, 4096);
	va_list ap;
	va_start(ap, fmt);
	int retval = vsprintf(temp_buf,fmt,ap);
	va_end(ap);
	if(is_static==0) {
		if((strlen(kmsg)+strlen(temp_buf)+1024)> (MAXBUF)) {
			memmove(kmsg,kmsg+strlen(temp_buf),strlen(temp_buf));
			memmove(kmsg+(MAXBUF)-strlen(temp_buf),temp_buf,strlen(temp_buf));
		} else {
			kmsg = realloc((void*)kmsg, strlen(kmsg)+strlen(temp_buf)+1024); // always tag on 1024kb so we don't need to reallocate every time
			strcat(kmsg,temp_buf);
		}
	} else {
		strcat(static_kmsg,temp_buf);
	}
	printf(temp_buf);
        return retval;
} 
