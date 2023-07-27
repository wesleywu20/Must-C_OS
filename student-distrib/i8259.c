/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"

#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/*
  * i8259_init
  *   DESCRIPTION: Initialize the 8259 PIC
  *   INPUTS: none
  *   OUTPUTS: master and slave PIC are initialized
  *            slave PIC is registerd to IRQ2 of master PIC
  *   RETURN VALUE: none
  *   SIDE EFFECTS: all interrupt lines are masked except the IRQ2 on master PIC for slave PIC
  */
void i8259_init(void) {
    outb(0xFF, MASTER_8259_DATA);  // mask everything during initalization
    outb(0xFF, SLAVE_8259_DATA);   // mask everything during initalization

    outb(ICW1, MASTER_8259_CMD);  // ICW1: select master 8259A-1 init
    outb(ICW1, SLAVE_8259_CMD);   // ICW1: select slave 8259A-1 init

    outb(ICW2_MASTER, MASTER_8259_DATA);  // ICW2: master PIC vector offset
    outb(ICW2_SLAVE, SLAVE_8259_DATA);    // ICW2: slave PIC vector offset

    outb(ICW3_MASTER, MASTER_8259_DATA);  // ICW3: tell master PIC there is slave PIC at IRQ2
    outb(ICW3_SLAVE, SLAVE_8259_DATA);    // ICW3 : tell slave PIC its cascade identity

    outb(ICW4, MASTER_8259_DATA);  // ICW4: additional setup for master PIC
    outb(ICW4, SLAVE_8259_DATA);   // ICW4: additional setup for slave PIC

    master_mask = 0xFB;                   // mask all of master 8259A except for IRQ2 (slave port)
    slave_mask = 0xFF;                    // mask all of slave 8259A
    outb(master_mask, MASTER_8259_DATA);  // set the mask for master PIC
    outb(slave_mask, SLAVE_8259_DATA);    // set the mask for slave PIC
}

/*
  * enable_irq
  *   DESCRIPTION: Enable (unmask) the specified IRQ 
  *   INPUTS: irq_num - irq line number to be enabled
  *   OUTPUTS: irq_num line is enabled on the PIC if it's within range
  *   RETURN VALUE: none
  *   SIDE EFFECTS: none
  */
void enable_irq(uint32_t irq_num) {
    // check if irq_num within range
    if (irq_num < MIN_IRQ_NUM || irq_num > MAX_IRQ_NUM) {
        printf("Invalid interrupt request line for enable_irq.\n");
        return;
    }
    uint16_t port = irq_num < IRQ_NUM_PER_PIC ? MASTER_8259_DATA : SLAVE_8259_DATA;  // check if request num came from master or slave pic

    if (irq_num < IRQ_NUM_PER_PIC) {  // if master, set all bits to 1 except for irq line on master mask
        master_mask &= ~(1 << irq_num);
    } else {  // otherwise, subtract 8 from irq number and perform same operation on slave mask
        slave_mask &= ~(1 << (irq_num - IRQ_NUM_PER_PIC));
    }
    uint8_t value = irq_num < IRQ_NUM_PER_PIC ? master_mask : slave_mask;  // choose the correct PIC based on irq_num
    outb(value, port);  // send new mask appropriately based on the request number
}

/*
  * disable_irq
  *   DESCRIPTION: Disable (mask) the specified IRQ 
  *   INPUTS: irq_num - irq line number to be disabled
  *   OUTPUTS: irq_num line is disabled on the PIC if it's within range
  *   RETURN VALUE: none
  *   SIDE EFFECTS: none
  */
void disable_irq(uint32_t irq_num) {
    // check if irq_num is within range
    if (irq_num < MIN_IRQ_NUM || irq_num > MAX_IRQ_NUM) {
        printf("Invalid interrupt request line for disable_irq.\n");
        return;
    }
    uint16_t port = irq_num < IRQ_NUM_PER_PIC ? MASTER_8259_DATA : SLAVE_8259_DATA;  // check if request num came from master or slave pic

    if (irq_num < IRQ_NUM_PER_PIC) {  // if master, set bit of request line on master mask to 1
        master_mask |= (1 << irq_num);
    } else {  // otherwise, subtract 8 from request line, then perform same operation as on master mask
        slave_mask |= (1 << (irq_num - IRQ_NUM_PER_PIC));
    }
    uint8_t value = irq_num < IRQ_NUM_PER_PIC ? master_mask : slave_mask; // choose the correct PIC based on irq_num
    outb(value, port);  // send new mask appropriately based on the request number
}

/*
  * send_eoi
  *   DESCRIPTION: Send end-of-interrupt signal for the specified IRQ 
  *   INPUTS: irq_num - irq line number to send EOI to
  *   OUTPUTS: EOI is sent to the correct irq_num
  *            if irq_num is from the master PIC, only master PIC receives EOI
  *            if irq_num is from the slave PIC, slave PIC receives EOI for that port, 
  *            and master PIC receives EOI for the port secandy PIC is on
  *   RETURN VALUE: none
  *   SIDE EFFECTS: none
  */
void send_eoi(uint32_t irq_num) {
    // check if irq_num is within range
    if (irq_num < MIN_IRQ_NUM || irq_num >= MAX_IRQ_NUM) {
        printf("Invalid interrupt request line for send_eoi.\n");
        return;
    }
    // interrupt came from slave PIC
    if (irq_num >= IRQ_NUM_PER_PIC) {
        // send EOI on slave PIC first, based on correct port on master PIC
        outb(EOI | (irq_num % IRQ_NUM_PER_PIC), SLAVE_8259_CMD);
        // send eoi for port occupied by slave PIC to master PIC
        outb(EOI | SLAVE_ON_MASTER_PORT, MASTER_8259_CMD);
    } else {
        // send EOI based on correct port on master PIC
        outb(EOI | irq_num, MASTER_8259_CMD);
    }
}
