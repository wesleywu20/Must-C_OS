#include "tests.h"

#ifdef CP4

#include "../syscall.h"
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
//file_descriptor_t TEST_PCB[PCB_SIZE];

/* Checkpoint 4 tests */
int execute_test(){
    TEST_HEADER;
    execute((uint8_t*)"shell 1234 9876");
    return PASS;
}

/* Test suite entry point */
void launch_tests_cp4() {
    // TEST_OUTPUT("execute_test", execute_test());
    clear();
}

#endif
