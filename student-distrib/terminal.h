#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"

// reads from the keyboard buffer into buf, return number of bytes read
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
// writes to the screen from buf, return number of bytes written or -1
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
// initializes terminal variables
int32_t terminal_open(const uint8_t* filename);
// clears terminal variables
int32_t terminal_close(int32_t fd);

#endif
