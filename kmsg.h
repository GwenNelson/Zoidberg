#ifndef KMSG_H
#define KMSG_H

extern char *kmsg;

void init_static_kmsg();

void init_dynamic_kmsg();

int kprintf(const char *fmt, ...);

#endif
