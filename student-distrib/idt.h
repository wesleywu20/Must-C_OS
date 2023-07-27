// idt.h - defines for various IDT related functions and constants
#ifndef _IDT_H
#define _IDT_H

#include "idt_linkage.h"

#define NUM_EXCEPTION 32
#define KERNEL_PRIVILEGE 0
#define USER_PRIVILEGE 3

// index of each entry in IDT
#define SYSCALL_INDEX 0x80
#define KEYBOARD_INDEX 0x21
#define RTC_INDEX 0x28
#define TIMER_INDEX 0x20

// initialize IDT
void init_IDT();

#endif
