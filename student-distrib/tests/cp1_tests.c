#include "tests.h"

#ifdef CP1

#include "../syscall_asm.h"
#include "../lib.h"
#include "../x86_desc.h"
#include "../i8259.h"
#include "../rtc.h"
#include "../terminal.h"
#include "../filesystem.h" 

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER \
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result) \
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure() {
    /* Use exception #15 for assertions, otherwise
       reserved by Intel */
    asm volatile("int $15");
}

// declare the global test PCB for cp2 (error if you don't put it)
file_descriptor_t TEST_PCB[PCB_SIZE];

/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test() {
    TEST_HEADER;

    int i;
    int result = PASS;
    for (i = 0; i < 10; ++i) {
        if ((idt[i].offset_15_00 == NULL) &&
            (idt[i].offset_31_16 == NULL)) {
            assertion_failure();
            result = FAIL;
        }
    }
    return result;
}

/* Divide by zero test
 *
 * Asserts that dividing by zero triggers the correct handler
 * which prints the error and loops infinitely without accepting other exceptions/interrupts
 * Inputs: None
 * Outputs: only "Divide Error Exception" is printed to screen
 * Side Effects: disables other exceptions/interrupts
 * Coverage: IDT definition
 * Files: idt.c
 */
int divide_by_zero_test() {
    TEST_HEADER;

    volatile int i = 0;
    i = (1 / i);  // triggers divide by zero exception

    // we should never reach below
    asm volatile("mov %cr6, %eax");  // triggers invalid opcode exception
    int result = FAIL;  // even though we set the result to FAIL, nothing should be printed to screen
    return result;
}


/* Invalid Opcode Test
 *
 * Asserts that accessing non-existent control register triggers the correct handler
 * which prints the error and loops infinitely without accepting other exceptions/interrupts
 * Inputs: None
 * Outputs: only "Invalid Opcode Exception" is printed to screen
 * Side Effects: disables other exceptions/interrupts
 * Coverage: IDT definition
 * Files: idt.c
 */
int invalid_opcode_test() {
    TEST_HEADER;

    asm volatile("mov %cr6, %eax");  // triggers invalid opcode exception
    // we should never reach below
    asm volatile("mov %cr6, %eax");  // triggers invalid opcode exception
    int result = FAIL; // even though we set the result to FAIL, nothing should be printed to screen
    return result;
}

/* System Call Test
 *
 * Asserts that making a system call triggers the correct handler
 * which prints "A system call is called" and continues
 * Inputs: None
 * Outputs: only "A system call is called" is printed to screen
 * Side Effects: disables other exceptions/interrupts
 * Coverage: IDT definition
 * Files: idt.c, syscall.c
 */
int system_call_test() {
    TEST_HEADER;

    asm volatile("int $128"); // triggers system call
    int result = PASS;        // if we can see the passing message, we know the handler exited correctly
    return result;
}

/*
  * PIC bound test
  *   DESCRIPTION: Assert that sending meaningless argument to PIC functions wouldn't crash the system
  *   INPUTS: none
  *   OUTPUTS: the system should handle the requests without crashing
  *   SIDE EFFECTS: none
  *   COVERAGE: i8259.c
  */
int pic_bound_test() {
    TEST_HEADER;
    int result = PASS;
    // sending multiple requests with out-of-bound value
    disable_irq(16);
    send_eoi(391);
    enable_irq(-1);

    return result;
}

/*
  * paging_nullptr_test
  *   DESCRIPTION: assert that dereferencing a null ptr would cause a page fault
  *   INPUTS: none
  *   OUTPUTS: the system should receive a page fault after dereferencing the null ptr
  *   SIDE EFFECTS: none
  *   COVERAGE: paging.c
  */
int paging_nullptr_test() {
    TEST_HEADER;
    int* nullptr = NULL;
    *nullptr = 6;  // dereferencing the nullptr
    // the code below should never be reached
    int result = FAIL;
    return result;
}

/*
 * paging_video_memory_test
 *   DESCRIPTION: assert that all of the memory address inside video memory won't cause a page fault 
 *   INPUTS: none
 *   OUTPUTS: the test passing message will be printed to screen
 *   SIDE EFFECTS: none
 *   COVERAGE: paging.c
 */
int paging_video_memory_test() {
    TEST_HEADER;

    unsigned int valid_addr = 0xb8000;
    char trash;
    for (; valid_addr < 0xb9000; valid_addr++){
        // no error should occur for all of video memory
        trash = *((char*)valid_addr);
    }

    // if we reach this point without page fault, the test passes
    int result = PASS;
    return result;
}
/*
 * paging_boundary_test
 *   DESCRIPTION: assert that the memory address close to the boundary of pages would cause a page fault
 *   INPUTS: none
 *   OUTPUTS: the system should receive a page fault if one of the boundaries is uncommented
 *   SIDE EFFECTS: none
 *   COVERAGE: paging.c
 */
int paging_boundary_test() {
    TEST_HEADER;

    // uncomment the boundary you'd like to check
    int* boundary1 = (int*)0xb7fff;
    *boundary1 = 6;  // dereferencing the boundary
    // int* boundary2 = 0xb9000;
    // *boundary2 = 6;  // dereferencing the boundary
    // int* boundary3 = 0x3fffff;
    // *boundary3 = 6;  // dereferencing the boundary
    // int* boundary4 = 0x800000;
    // *boundary4 = 6;  // dereferencing the boundary

    // the code below should never be reached if one of the boundaries is uncommented
    int result = PASS;
    return result;
}

/*
  * paging_nonexistent_page_test
  *   DESCRIPTION: assert that accessing a nonexistent page would cause a page fault
  *   INPUTS: none
  *   OUTPUTS: the system should receive a page fault
  *   SIDE EFFECTS: none
  *   COVERAGE: paging.c
  */
int paging_nonexistent_page_test() {
    TEST_HEADER;
    int* thirdPageAddress = (int*)(0x400000 * 2 + 1234);  // choose a random address in the third page
    *thirdPageAddress = 6; // dereference that address should give page fault
    // the code below should never be reached
    int result = FAIL;
    return result;
}

/* Test suite entry point */
void launch_tests_cp1() {
    clear();

    // checkpoint 1 tests
    // TEST_OUTPUT("idt_test", idt_test());
    // TEST_OUTPUT("invalid_opcode_test", invalid_opcode_test());
    // TEST_OUTPUT("divide_by_zero_test", divide_by_zero_test());
    TEST_OUTPUT("system_call_test", system_call_test());
	// TEST_OUTPUT("pic_bound_test", pic_bound_test());
	// TEST_OUTPUT("paging_video_memory_test", paging_video_memory_test());
	// TEST_OUTPUT("paging_nullptr_test", paging_nullptr_test());
	// TEST_OUTPUT("paging_boundary_test", paging_boundary_test());
	// TEST_OUTPUT("paging_nonexistent_page_test", paging_nonexistent_page_test());

}

#endif
