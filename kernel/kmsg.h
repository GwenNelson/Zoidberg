#ifndef KMSG_H
#define KMSG_H

#define KLOG_ERR  0  // this log entry is an error
#define KLOG_OK   1  // this log entry is just a status update, it's all good
#define KLOG_PROG 2  // this log entry is something that needs a progress bar

#ifndef IN_KMSG
extern char *kmsg;
#endif

void init_static_kmsg();

void init_dynamic_kmsg();

int kprintf(const char *fmt, ...);

int klog(char* component, int is_good, const char *fmt, ...);

// pass in something like bytes in a file to read etc
void kmsg_prog_start(UINT64 total);

// pass in a number of bytes just read (or whatever) to increment progress by
void kmsg_prog_update(UINT64 n);

#endif
