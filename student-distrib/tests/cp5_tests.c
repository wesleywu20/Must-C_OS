#include "tests.h"

#ifdef CP5

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

/* Checkpoint 5 tests */

/*
 * execute_garbage_input_test
 *   DESCRIPTION: bad input to function
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: returns failure
 *   COVERAGE: syscall.c -- syscall_execute
 */
int execute_garbage_input_test() {
    TEST_HEADER;
    int result = execute((uint8_t*)"abcdefg");
    if (result != -1) {
        return FAIL;
    }
    return result;
}

/* Test suite entry point */
void launch_tests_cp5() {
    clear();
    // TEST_OUTPUT("execute garbage input test", execute_garbage_input_test());
}

#endif
