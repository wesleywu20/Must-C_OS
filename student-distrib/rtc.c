#include "rtc.h"

#include "tests/tests.h"
#include "types.h"
#include "filesystem.h"
#include "pcb.h"

// local functions
void _rtc_interrupt_handler();

// local variables
uint8_t flag = 1;

// rtc operations table
const fileops_table_t rtcOps_table = {
    .fd_open = &rtc_open,
    .fd_read = &rtc_read,
    .fd_write = &rtc_write,
    .fd_close = &rtc_close,
};
/*
 * init_rtc
 *   DESCRIPTION: initializes rtc
 *   INPUTS: none
 *   OUTPUTS: rtc is initialized and starts generating interrupt with default frequency 1024 Hz
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_rtc() {
    // steps from OSDev (turning on IRQ 8)
    outb(REGISTER_B, RTC_INDEX_PORT);  // select register B, and disable NMI
    char prev = inb(RTC_DATA_PORT);    // read the current value of register B
    outb(REGISTER_B, RTC_INDEX_PORT);  // set the index again (a read will reset the index to register D)
    outb(prev | 0x40, RTC_DATA_PORT);  // write the previous value ORed with 0x40. This turns on bit 6 of register B
    enable_irq(RTC_IRQ_NUM);           // enable rtc
}

/*
 * _rtc_interrupt_handler
 *   DESCRIPTION: handler for rtc, which will be used for assembly linkage in idt_linkage.S
 *   INPUTS: none
 *   OUTPUTS: rtc interrupt correctly handled
 *            for cp1, the whole screen will be filled with characters because of test_interrupts
 *   RETURN VALUE: none
 *   SIDE EFFECTS: other handlers cannot run simultaneously because of critical section
 */
void _rtc_interrupt_handler() {
    cli();  // start critical section
#ifdef rtc_CP1
    test_interrupts();  // for cp1 only
#endif
    outb(REGISTER_C, RTC_INDEX_PORT);  // select register C
    (void)inb(RTC_DATA_PORT);          // just throw away contents
    send_eoi(RTC_IRQ_NUM);             // send eoi to IRQ8, the port rtc occupies on PIC
    flag = 1;
    sti();  // end critical section
}

/*
 * rtc_read
 *   DESCRIPTION: blocks until next rtc interrupt
 *   INPUTS: fd - file descriptor
 *           buf - buffer to read from (ignored)
 *           nbytes - bytes to be read
 *   OUTPUTS: blocks the program until next rtc interrupt has occured
 *   RETURN VALUE: 0 for successful operation
 *   SIDE EFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    flag = 0; // set the flag to 0
    while (flag == 0) { // wait until the flag is no longer 0, which can only be changed by keyboard interrupt handler
    };
    return 0;
}

/*
 * rtc_write
 *   DESCRIPTION: change the frequency of rtc
 *   INPUTS: fd - file descriptor
 *           buf - buffer to read from
 *           nbytes - bytes to be read
 *   OUTPUTS: frequecy of rtc changed based on input read from buf
 *   RETURN VALUE: 0 for successful operation
 *                 -1 if frequency is not power of 2, or too fast, or too slow
 *   SIDE EFFECTS: none
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    int frequency = *((int*)buf);            // get frequency from terminal buffer
    if ((frequency & (frequency - 1)) != 0)  // not power of 2 is bad input
        return -1;

    // use the formula calculate rate to set from frequency input
    // frequency = 32768 >> (rate-1)
    //           = 2^(15-rate+1)
    //           = 2^(16-rate)
    //           = 2^16 / 2^rate
    // 2^rate = 65536 / frequency
    // which means rate is the index of 1 in binary (65536/frequency)
    int twoPowRate = (65536 / frequency); 
    uint8_t rate = 0;
    while (twoPowRate != 0) {
        // base case, we have reached twoPowRate == 1
        // rate is the index of that 1 in binary
        if (twoPowRate == 1) {
            break;
        }
        // for each iteration, shift twoPowRate by 1 bit and increment rate
        twoPowRate >>= 1;
        rate += 1;
    }
    if (rate < 6 || rate > 15)  // rate greater than 1024 Hz or invalid
        return -1;

    cli();
    outb(REGISTER_A, RTC_INDEX_PORT);           // set index to register A, disable NMI
    char prev = inb(RTC_DATA_PORT);             // get initial value of register A
    outb(REGISTER_A, RTC_INDEX_PORT);           // reset index to A
    outb((prev & 0xF0) | rate, RTC_DATA_PORT);  // write only our rate to A. Note, rate is the bottom 4 bits.

    outb(REGISTER_A, RTC_INDEX_PORT);  // set index to register A, disable NMI
    prev = inb(RTC_DATA_PORT);         // get initial value of register A
    sti();
    return 0;
}

/*
 * rtc_open
 *   DESCRIPTION: open the rtc device and changes the frequency to the default of 2 Hz
 *   INPUTS: filename - name of the device file
 *   OUTPUTS: frequency of rtc changed to 2 Hz, rtc opened in PCB
 *   RETURN VALUE: fd for successful operation, 
 *                 -1 for full PCB
 *   SIDE EFFECTS: none
 */
int32_t rtc_open(const uint8_t* filename) {
    // make sure filename is valid
    if (filename == NULL) {
        return -1;
    }
    int fd;
    dentry_t dir_entry;
    // check for empty space in PCB
    // start from 2 since stdin and stdout always take TEST_PCB[0] and TEST_PCB[1]
    for (fd = 2; fd < PCB_SIZE; fd++) {
        // if rtc was already opened, use that fd index
        if (curr_pcb->file_desc[fd].fileops_table_ptr == (int32_t *)(&rtcOps_table))
            return fd;
        // if full (use flags to check) go to next spot
        if (curr_pcb->file_desc[fd].flags) 
            continue;
        else {
            // if empty fill in fd attributes with file attributes after finding dentry
            if (!read_dentry_by_name(filename, &dir_entry)) {
                curr_pcb->file_desc[fd].inode = dir_entry.inode_num;
                curr_pcb->file_desc[fd].fileops_table_ptr = (int32_t *)(&rtcOps_table);
                curr_pcb->file_desc[fd].file_pos = 0;
                curr_pcb->file_desc[fd].flags = 1;

                uint8_t rate = 0xF;  // 0xF corresponds to 2Hz according to ds12887 reference sheet
                cli();

                outb(REGISTER_A, RTC_INDEX_PORT);           // set index to register A, disable NMI
                char prev = inb(RTC_DATA_PORT);             // get initial value of register A
                outb(REGISTER_A, RTC_INDEX_PORT);           // reset index to A
                outb((prev & 0xF0) | rate, RTC_DATA_PORT);  // write only our rate to A. Note, rate is the bottom 4 bits.

                sti();
                // return fd just made
                return fd;
            }
            break;
        }
    }
    // no empty spots, more files cannot be opened, return -1 for failure
    return -1;
}

/*
 * rtc_close
 *   DESCRIPTION: closes the file descriptor associated with the rtc, if it was opened
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful operation
 *   SIDE EFFECTS: opens a slot on the current PCB's file descriptor array
 */
int32_t rtc_close(int32_t fd) {
    // check if the fd is valid and was open
    if ((fd >= 2 && fd < FD_SIZE) && curr_pcb->file_desc[fd].flags) {
        curr_pcb->file_desc[fd].inode = 0;
        curr_pcb->file_desc[fd].fileops_table_ptr = NULL;
        curr_pcb->file_desc[fd].file_pos = 0;
        curr_pcb->file_desc[fd].flags = 0;
    }
    return 0;
}
