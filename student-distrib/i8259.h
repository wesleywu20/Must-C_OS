/* i8259.h - Defines used in interactions with the 8259 interrupt
 * controller
 * vim:ts=4 noexpandtab
 */

#ifndef _I8259_H
#define _I8259_H

#include "types.h"

/* Ports that each PIC sits on */
#define MASTER_8259_PORT 0x20
#define SLAVE_8259_PORT 0xA0

#define MASTER_8259_CMD MASTER_8259_PORT
#define MASTER_8259_DATA MASTER_8259_PORT + 1
#define SLAVE_8259_CMD SLAVE_8259_PORT
#define SLAVE_8259_DATA SLAVE_8259_PORT + 1

#define SLAVE_ON_MASTER_PORT 2  // slave on IRQ2 of master PIC
#define MIN_IRQ_NUM 0           // min irq line number
#define MAX_IRQ_NUM 15          // max irq line number
#define IRQ_NUM_PER_PIC 8       // each PIC has 8 IRQ lines

/* Initialization control words to init each PIC.
 * See the Intel manuals for details on the meaning
 * of each word */
#define ICW1 0x11
#define ICW2_MASTER 0x20
#define ICW2_SLAVE 0x28
#define ICW3_MASTER 0x04
#define ICW3_SLAVE 0x02
#define ICW4 0x01

#define RTC_IRQ_NUM 8
#define KEYBOARD_IRQ_NUM 1
#define TIMER_IRQ_NUM 0

/* End-of-interrupt byte.  This gets OR'd with
 * the interrupt number and sent out to the PIC
 * to declare the interrupt finished */
#define EOI 0x60

/* Externally-visible functions */

/* Initialize both PICs */
void i8259_init(void);
/* Enable (unmask) the specified IRQ */
void enable_irq(uint32_t irq_num);
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num);
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num);

#endif /* _I8259_H */
