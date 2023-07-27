#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "types.h"
#include "pcb.h"

#define READ 3
#define WRITE 4
#define OPEN 5
#define CLOSE 6

#define MAX_ARG_NUM 5
#define MAX_BUF_SIZE 128

// system calls
extern int32_t halt(uint8_t status);
extern int32_t execute(const uint8_t* command);
extern int32_t open(const uint8_t * filename);
extern int32_t read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t write(int32_t fd, const void *buf, int32_t nbytes);
extern int32_t close(int32_t fd);
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap (uint8_t** screen_start);
extern int32_t set_handler (int32_t signum, void* handler_address);
extern int32_t sigreturn (void);
extern int32_t system_call_handler();

#endif
