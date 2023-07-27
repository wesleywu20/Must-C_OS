// idt.c - code related to IDT (interrupt descriptor table)
#include "idt.h"

#include "idt_linkage.h"
#include "lib.h"
#include "x86_desc.h"
#include "syscall.h"

// local functions (handlers)
void divide_error_exception_handler();
void debug_exception_handler();
void nmi_interrupt_handler();
void breakpoint_exception_handler();
void overflow_exception_handler();
void bound_range_exceeded_exception_handler();
void invalid_opcode_exception_handler();
void device_not_available_exception_handler();
void double_fault_exception_handler();
void coprocessor_segment_overrun_handler();
void invalid_tss_exception_handler();
void segment_not_present_handler();
void stack_fault_exception_handler();
void general_protection_exception_handler();
void page_fault_exception_handler();
void x87_FPU_floating_point_error_handler();
void alignment_check_exception_handler();
void machine_check_exception_handler();
void SIMD_floating_point_exception_handler();
void general_exception_handler();

/*
 * init_IDT
 *   DESCRIPTION: initalize IDT
 *   INPUTS: none
 *   OUTPUTS: IDT entry for exceptions, rtc, keyboard and system call is initalized
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_IDT() {
    // load the IDTR
    lidt(idt_desc_ptr);
    int i;  // loop index
    for (i = 0; i < NUM_VEC; i++) {
        // set the bits correctly for all entries in IDT
        // these are the mostly the same, except dpl for system call entry
        idt[i].present = 0;                                                   // present will be set to 1 by SET_IDT_ENTRY call
        idt[i].seg_selector = KERNEL_CS;                                      // choose code segment for kernel
        idt[i].dpl = i == SYSCALL_INDEX ? USER_PRIVILEGE : KERNEL_PRIVILEGE;  // set up privilege based on entry number
        idt[i].reserved0 = 0;                                                 // fixed based on intel manual
        idt[i].size = 1;                                                      // use 16 bits IDT entries
        idt[i].reserved1 = 1;                                                 // fixed based on intel manual
        idt[i].reserved2 = 1;                                                 // fixed based on intel manual
        idt[i].reserved3 = 0;                                                 // fixed based on intel manual
        idt[i].reserved4 = 0;                                                 // fixed based on intel manual
    }
    // register handlers for all exceptions based on intel IA-32 manual
    // modified SET_IDT_ENTRY to set the present field to 1
    SET_IDT_ENTRY(idt[0], &divide_error_exception_handler);
    SET_IDT_ENTRY(idt[1], &debug_exception_handler);
    SET_IDT_ENTRY(idt[2], &nmi_interrupt_handler);
    SET_IDT_ENTRY(idt[3], &breakpoint_exception_handler);
    SET_IDT_ENTRY(idt[4], &overflow_exception_handler);
    SET_IDT_ENTRY(idt[5], &bound_range_exceeded_exception_handler);
    SET_IDT_ENTRY(idt[6], &invalid_opcode_exception_handler);
    SET_IDT_ENTRY(idt[7], &device_not_available_exception_handler);
    SET_IDT_ENTRY(idt[8], &double_fault_exception_handler);
    SET_IDT_ENTRY(idt[9], &coprocessor_segment_overrun_handler);
    SET_IDT_ENTRY(idt[10], &invalid_tss_exception_handler);
    SET_IDT_ENTRY(idt[11], &segment_not_present_handler);
    SET_IDT_ENTRY(idt[12], &stack_fault_exception_handler);
    SET_IDT_ENTRY(idt[13], &general_protection_exception_handler);
    SET_IDT_ENTRY(idt[14], &page_fault_exception_handler);
    // putting a general interrupt for reserved entry 15
    SET_IDT_ENTRY(idt[15], &general_exception_handler);
    SET_IDT_ENTRY(idt[16], &x87_FPU_floating_point_error_handler);
    SET_IDT_ENTRY(idt[17], &alignment_check_exception_handler);
    SET_IDT_ENTRY(idt[18], &machine_check_exception_handler);
    SET_IDT_ENTRY(idt[19], &SIMD_floating_point_exception_handler);
    // putting a general interrupt for reserved entry 20 thru 31
    for (i = 20; i < NUM_EXCEPTION; i++) {
        SET_IDT_ENTRY(idt[i], &general_exception_handler);
    }
    // register system call handler
    SET_IDT_ENTRY(idt[SYSCALL_INDEX], &system_call_handler);
    // register keyboard, rtc, and timer handlers
    SET_IDT_ENTRY(idt[KEYBOARD_INDEX], &keyboard_interrupt_handler);
    SET_IDT_ENTRY(idt[RTC_INDEX], &rtc_interrupt_handler);
    SET_IDT_ENTRY(idt[TIMER_INDEX], &timer_handler);
}

/*
 * the rest of this file are handlers for exceptions
 * some_exception_handler
 *   DESCRIPTION: handler for a specific exception
 *                it prints the exception type and halts the program with an infinite loop
 *                it is executed in a critical section, which prevents other handlers from executing
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: program halts with an infinite loop
 *                 no other interrupts are processed
 */
void divide_error_exception_handler() {
    cli();
    printf("Divide Error Exception\n");
    sti();
    halt(-1);
}

void debug_exception_handler() {
    cli();
    printf("Debug Exception\n");
    sti();
    halt(-1);
}

void nmi_interrupt_handler() {
    cli();
    printf("NMI Interrupt\n");
    sti();
    halt(-1);
}

void breakpoint_exception_handler() {
    cli();
    printf("Breakpoint Exception\n");
    sti();
    halt(-1);
}

void overflow_exception_handler() {
    cli();
    printf("Overflow Exception\n");
    sti();
    halt(-1);
}

void bound_range_exceeded_exception_handler() {
    cli();
    printf("BOUND Range Exceeded Exception\n");
    sti();
    halt(-1);
}

void invalid_opcode_exception_handler() {
    cli();
    printf("Invalid Opcode Exception\n");
    sti();
    halt(-1);
}

void device_not_available_exception_handler() {
    cli();
    printf("Device Not Available Exception\n");
    sti();
    halt(-1);
}

void double_fault_exception_handler() {
    cli();
    printf("Double Fault Exception\n");
    sti();
    halt(-1);
}

void coprocessor_segment_overrun_handler() {
    cli();
    printf("Coprocessor Segment Overrun\n");
    sti();
    halt(-1);
}

void invalid_tss_exception_handler() {
    cli();
    printf("Invalid TSS Exception\n");
    sti();
    halt(-1);
}

void segment_not_present_handler() {
    cli();
    printf("Segment Not Present\n");
    sti();
    halt(-1);
}

void stack_fault_exception_handler() {
    cli();
    printf("Stack Fault Exception\n");
    sti();
    halt(-1);
}

void general_protection_exception_handler() {
    cli();
    printf("General Protection Exception\n");
    sti();
    halt(-1);
}

void page_fault_exception_handler() {
    cli();
    printf("Page Fault Exception\n");
    sti();
    halt(-1);
}

void x87_FPU_floating_point_error_handler() {
    cli();
    printf("X87 FPU Floating Point Error\n");
    sti();
    halt(-1);
}

void alignment_check_exception_handler() {
    cli();
    printf("Alignment Check Exception\n");
    sti();
    halt(-1);
}

void machine_check_exception_handler() {
    cli();
    printf("Machine Check Exception\n");
    sti();
    halt(-1);
}

void SIMD_floating_point_exception_handler() {
    cli();
    printf("SIMD Floating Point Exception\n");
    sti();
    halt(-1);
}

void general_exception_handler() {
    cli();
    printf("General Exception\n");
    sti();
    halt(-1);
}

