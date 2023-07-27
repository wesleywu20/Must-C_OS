#include "tests.h"

#ifdef CP3

#include "../syscall.h"
#include "../lib.h"
#include "../x86_desc.h"
#include "../i8259.h"
#include "../rtc.h"
#include "../terminal.h"
#include "../filesystem.h" 
#include "../paging.h"
#include "../pcb.h"

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
file_descriptor_t TEST_PCB[FD_SIZE];

/* Checkpoint 3 tests */

/* System Call Test
 *
 * Asserts that making a system call triggers the correct handler
 * which prints "A system call is called" and continues
 * Inputs: None
 * Outputs: only "A system call is called" is printed to screen
 * Side Effects: disables other exceptions/interrupts
 * Coverage: IDT definition
 * Files: idt.c, idt_linkage.S, syscall.c, syscall_asm.S, rtc.c, filesystem.c
 */
int improved_system_call_test() {
    TEST_HEADER;

    printf("\n");
    // write
    write(1, "testing writing to screen using write().\n\n", 42);
    // read
    printf("testing reading from terminal using read()\n");
    printf("start typing: ");
    char buf[128];
    read(0, buf, 128);
    printf("buf: %s\n", buf);
    // open
    printf("testing opening rtc\n");
    int fd = open((uint8_t*)"rtc");
    printf("rtc at fd = %d\n\n", fd);
    if (fd != 2) {
        return FAIL;
    }
    // close
    printf("testing closing rtc\n");
    int res = close(fd);
    printf("rtc_close return value = %d\n\n", res);
    if (res != 0) {
        return FAIL;
    }
    return PASS;  // if we have reached this point, the code is functional
}

/*
 * find_avail_pid_test
 *   DESCRIPTION: given that index 0 is taken, test 
 *      that process will be assigned to pid 1
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: assigns process pid 1 and makes 
 *      pid 1 unavailable
 *   COVERAGE: pcb.c -- find_avail_pid
 */
int find_avail_pid_test() {
    int pid;
    pcb_arr[0]->available = 0; // set unavail
    pcb_arr[1]->available = 1; // set avail
    pid = find_avail_pid(); // find avail pid
    if((pid == 1) && (pcb_arr[pid]->process_id == 1))
        return PASS; // pid marked unavail
    else
        return FAIL; // could not find avail pid
}

/*
 * neither_pid_avail_test
 *   DESCRIPTION: return -1 if neither pids are available
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: cannot assign pid
 *   COVERAGE: pcb.c -- find_avail_pid
 */
int neither_pid_avail_test() {
    int pid;
    int i;
    for (i = 0; i < 6; i++) {
        pcb_arr[i]->available = 0; // set unavail
    }
    pid = find_avail_pid();
    if(pid == -1) // cant find pid, return PASS
        return PASS;
    else
        return FAIL;
}

/*
 * clear_pid_test
 *   DESCRIPTION: sets unavailable pid to available
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: clears pid 0
 *   COVERAGE: pcb.c -- clear_pid
 */
int clear_pid_test() {
    pcb_arr[0]->available = 0; // set unavail
    int val = clear_pid(0); // clear pid 0
    if(val == 0) // return 0 if clear success
        return PASS;
    return FAIL;
}

/*
 * clear_pid_bad_input_test
 *   DESCRIPTION: sends in invalid input to clear_pid
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: cannot clear pid
 *   COVERAGE: pcb.c -- clear_pid
 */
int clear_pid_bad_input_test() {
    int val = clear_pid(7); // give bad input
    printf("%d\n", val);
    if(val == -1) // should fail
        return PASS;
    return FAIL;
}

/*
 * create_pcb_test
 *   DESCRIPTION: creates a pcb
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: creates a pcb
 *   COVERAGE: pcb.c -- create_pcb
 */
int create_pcb_test() {
    uint32_t fake_ebp, fake_esp;
    int fake_parent, val;
    pcb_arr_init(); // init pid_arr so both are avail
    fake_parent = -1;
    fake_ebp = 0x00000001;
    fake_esp = 0x00000010;
    val = create_pcb(fake_parent, fake_ebp, fake_esp); // create pcb 
    printf("%d\n", pcb_arr[val]->process_id);
    if (!((val == 0) && (pcb_arr[val]->process_id == 0) && (pcb_arr[val]->parent_id == fake_parent) && (pcb_arr[val]->saved_ebp == fake_ebp) && (pcb_arr[val]->saved_esp == fake_esp)))
        return FAIL; // if all the args and pid match then PASS
    return PASS;
}

/*
 * point_curr_pcb_test
 *   DESCRIPTION: points curr_pcb to one of the PCBs
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: points curr_pcb to PCB
 *   COVERAGE: pcb.c -- point_curr_pcb
 */
int point_curr_pcb_test() {
    uint32_t fake_ebp, fake_esp;
    int fake_parent, val, val1, val2;
    fake_parent = -1;
    pcb_arr_init(); // init pid_arr so both are avail
    fake_ebp = 0x00000001;
    fake_esp = 0x00000010;
    val1 = create_pcb(fake_parent, fake_ebp, fake_esp); // create pcb1
    val = point_curr_pcb(0); // point curr_pcb to pcb1
    printf("curr_pcb address: %d\n", (uint32_t)curr_pcb);
    printf("PCB1_POS: %d\n", PCB1_POS);
    if((val == 0) && ((uint32_t)curr_pcb != PCB1_POS)) // make sure pointing returned success and correct address
        return FAIL;
    val2 = create_pcb(fake_parent, fake_ebp, fake_esp); // create pcb2
    val = point_curr_pcb(1); // point curr_pcb to pcb2
    printf("curr_pcb address: %d\n", (uint32_t)curr_pcb);
    printf("PCB2_POS: %d\n", PCB2_POS);
    if((val == 1) && ((uint32_t)curr_pcb != PCB2_POS)) // make sure pointing returned success and correct address
        return FAIL;
    return PASS; // if reached then PASS
}

/*
 * point_curr_pcb_bad_input_test
 *   DESCRIPTION: bad input to function
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: returns failure
 *   COVERAGE: pcb.c -- point_curr_pcb
 */
int point_curr_pcb_bad_input_test() {
    int val = point_curr_pcb(3); // bad input
    if(val == -1) // should fail
        return PASS;
    return FAIL;
}

/*
 * execute_test
 *   DESCRIPTION: assert that execute would run the program correctly
 *   INPUTS: none
 *   OUTPUTS: shell starts running
 *   RETURN VALUE: 0 for a successful execute
 *   SIDE EFFECTS: none
 */
int execute_test(){
    TEST_HEADER;
    execute((uint8_t*)"shell 1234 9876");
    return PASS;
}

/* Test suite entry point */
void launch_tests_cp3() {
    clear();
    // TEST_OUTPUT("find_avail_pid_test", find_avail_pid_test());
    // TEST_OUTPUT("neither_pid_avail_test", neither_pid_avail_test());
    // TEST_OUTPUT("clear_pid_test", clear_pid_test());
    // TEST_OUTPUT("clear_pid_bad_input_test", clear_pid_bad_input_test());
    // TEST_OUTPUT("point_curr_pcb_test", point_curr_pcb_test());
    // TEST_OUTPUT("point_curr_pcb_bad_input_test", point_curr_pcb_bad_input_test());
    // TEST_OUTPUT("create_pcb_test", create_pcb_test());
    // TEST_OUTPUT("improved system_call_test", improved_system_call_test());
    // TEST_OUTPUT("execute_test", execute_test());
}

#endif
