#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

char *kmsg=NULL;

static char static_kmsg[4096];
static int is_static=1;
static int buf_len=4096;


void init_static_kmsg() {
     memset((void*)static_kmsg,0,4096);
}

int kprintf(const char *fmt, ...);
void init_dynamic_kmsg() {
	kprintf("kmsg: about to switch to dynamic buffer, if things go silent after this we have memory issues\n");
	kmsg = realloc((void*)kmsg, 4096);
	memset((void*)kmsg, 0, 4096);
	strcat(kmsg,static_kmsg);
	kprintf("kmsg: allocated dynamic buffer!\n");
	is_static = 0;
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
		if((strlen(temp_buf)+strlen(kmsg))>buf_len) {
			kmsg = realloc((void*)kmsg, strlen(kmsg)+strlen(temp_buf)+8192); // always tag on a few kb so we don't need to reallocate every time
			buf_len = strlen(kmsg)+strlen(temp_buf)+8192;
		}
		strcat(kmsg,temp_buf);
	} else {
		strcat(static_kmsg,temp_buf);
	}
	printf(temp_buf);
        return retval;
} 
