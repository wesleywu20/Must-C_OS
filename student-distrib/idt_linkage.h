#ifndef _IDT_LINKAGE_H
#define _IDT_LINKAGE_H

// all the handlers that have used assembly linkage
extern void keyboard_interrupt_handler();
extern void rtc_interrupt_handler();
extern int system_call_handler();
extern void timer_handler();

#endif
