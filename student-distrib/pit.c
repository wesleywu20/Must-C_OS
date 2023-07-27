#include "pit.h"

volatile int counter;
void _timer_handler();

/*
 * init_timer
 *    DESCRIPTION: initializes PIT chip
 *    INPUTS: freq -- the frequency (in Hz) to initialize the timer to
 *    OUTPUTS: the PIT is initialized and the counter is initialized to 0 
 *    RETURN VALUE: none
 *    SIDE EFFECTS: none
 */
void init_timer(int freq) {
    counter = 0; // init counter
    int divisor = PIT_OSC_FREQ_HZ / freq; // calculate tick divider
    outb(CH0_MODE2_BYTE, MODE_CMD_REG); // send byte to mode/cmd reg to access both lobyte/hibyte of channel 0 in mode 2
    outb(divisor & LOBYTE_MASK, CH0_DATA_PORT); // send lobyte of channel 0 to channel 0 data port
    outb(divisor >> HIBYTE_SHIFT, CH0_DATA_PORT); // send hibyte of channel 0 to channel 0 data port
    enable_irq(TIMER_IRQ_NUM); // unmask IRQ0 for the timer chip to allow timer chip to send interrupts
}

/*
 * _timer_handler
 *   DESCRIPTION: handler for PIT which is passed into assembly linkage in idt_linkage.S
 *                executed in a critical section
 *   INPUTS: none
 *   OUTPUTS: the scheduler will be called every time interval given by frequency passed into init
 *   RETURN VALUE: none
 *   SIDE EFFECTS: other interrupt handlers cannot run simultaneously because of critical section
 */
void _timer_handler() {
    cli();
    counter = (counter + 1) % 3; // increment counter
    // TODO: call scheduler program, implement scheduling
    send_eoi(TIMER_IRQ_NUM); // send eoi to IRQ0, port that timer chip occupies on PIC
    sti();
}
