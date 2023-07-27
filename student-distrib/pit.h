#ifndef _PIT_H
#define _PIT_H

#define CH0_DATA_PORT 0x40
#define CH1_DATA_PORT 0x41
#define CH2_DATA_PORT 0x42
#define MODE_CMD_REG 0x43

#include "types.h"
#include "lib.h"
#include "i8259.h"

/*
 * Mode/Command Register Bits
 *
 * Bits         Usage
 * 6 and 7      Select channel :
 *                 0 0 = Channel 0
 *                 0 1 = Channel 1
 *                 1 0 = Channel 2
 *                 1 1 = Read-back command (8254 only - see below)
 * 4 and 5      Access mode :
 *                 0 0 = Latch count value command
 *                 0 1 = Access mode: lobyte only
 *                 1 0 = Access mode: hibyte only
 *                 1 1 = Access mode: lobyte/hibyte
 * 1 to 3       Operating mode :
 *                 0 0 0 = Mode 0 (interrupt on terminal count)
 *                 0 0 1 = Mode 1 (hardware re-triggerable one-shot)
 *                 0 1 0 = Mode 2 (rate generator)
 *                 0 1 1 = Mode 3 (square wave generator)
 *                 1 0 0 = Mode 4 (software triggered strobe)
 *                 1 0 1 = Mode 5 (hardware triggered strobe)
 *                 1 1 0 = Mode 2 (rate generator, same as 010b)
 *                 1 1 1 = Mode 3 (square wave generator, same as 011b)
 * 0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
 *
 * Mode/Command Register Read-back command
 * 
 * Bits         Usage
 * 7 and 6      Must be set for the read back command
 * 5            Latch count flag (0 = latch count, 1 = don't latch count)
 * 4            Latch status flag (0 = latch status, 1 = don't latch status)
 * 3            Read back timer channel 2 (1 = yes, 0 = no)
 * 2            Read back timer channel 1 (1 = yes, 0 = no)
 * 1            Read back timer channel 0 (1 = yes, 0 = no)
 * 0            Reserved (should be clear)
 * 
 * If bit 5 is low in the read-back command, the format of the status byte returned is as such:
 * Bit/s        Usage
 * 7            Output pin state
 * 6            Null count flags
 * 4 and 5      Access mode :
 *                 0 0 = Latch count value command
 *                 0 1 = Access mode: lobyte only
 *                 1 0 = Access mode: hibyte only
 *                 1 1 = Access mode: lobyte/hibyte
 * 1 to 3       Operating mode :
 *                 0 0 0 = Mode 0 (interrupt on terminal count)
 *                 0 0 1 = Mode 1 (hardware re-triggerable one-shot)
 *                 0 1 0 = Mode 2 (rate generator)
 *                 0 1 1 = Mode 3 (square wave generator)
 *                 1 0 0 = Mode 4 (software triggered strobe)
 *                 1 0 1 = Mode 5 (hardware triggered strobe)
 *                 1 1 0 = Mode 2 (rate generator, same as 010b)
 *                 1 1 1 = Mode 3 (square wave generator, same as 011b)
 * 0            BCD/Binary mode: 0 = 16-bit binary, 1 = four-digit BCD
*/

#define PIT_OSC_FREQ_HZ 1193180

/* 
 * CH0_MODE2_BYTE = 00110100
 * CH0_MODE2_BYTE[7:6] = 00b - channel 0
 * CH0_MODE2_BYTE[5:4] = 11b - acces mode: lobyte/hibyte
 * CH0_MODE2_BYTE[3:1] = 010b - mode 2
 * CH0_MODE2_BYTE[0] = 0b - 16-bit binary mode
 */
#define CH0_MODE2_BYTE 0x36

#define LOBYTE_MASK 0xFF
#define HIBYTE_SHIFT 8

void init_timer(int freq);

#endif
