#ifndef K_CONSOLE_H
#define K_CONSOLE_H

#include "libvterm/vterm.h"

void init_console();
void console_write_chars(char* chars, size_t len);

#endif
