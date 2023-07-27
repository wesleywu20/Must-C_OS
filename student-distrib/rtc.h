#ifndef _RTC_H
#define _RTC_H

#include "types.h"
#include "lib.h"
#include "i8259.h"

#define RTC_INDEX_PORT 0x70
#define RTC_DATA_PORT 0x71
#define REGISTER_A  0x8A
#define REGISTER_B  0x8B
#define REGISTER_C  0x0C

// initializes rtc
void init_rtc();

// blocks until next rtc interrupt
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

// change the frequency of rtc
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

// open the rtc device and changes the frequency to the default of 2 Hz
int32_t rtc_open(const uint8_t* filename);

// does nothing
int32_t rtc_close(int32_t fd);

#endif
