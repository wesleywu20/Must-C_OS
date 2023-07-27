#ifndef __TERMINALS_H
#define __TERMINALS_H

#include "paging.h"
#include "pcb.h"

#define NUM_TERMINALS 3 /* max number of terminals */

// struct for storing a terminal's information
typedef struct terminal {
    int initialized;     // whether terminal has been initialized
    pcb_t* curr_pcb;     // current pcb
    int cursor_x;        // x position of cursor
    int cursor_y;        // y position of cursor
    uint32_t ebp;        // saved ebp
    uint32_t esp;        // saved esp
    uint32_t eip;        // saved eip
    uint32_t esp0;       // saved esp0
    uint32_t ss0;        // saved ss0
    int is_kernel_mode;  // whether terminal is in kernel mode
    // TODO: input buffer
} terminal_t;

// global array of terminal information
extern terminal_t terminal_arr[NUM_TERMINALS];

// current running terminal
extern int curr_terminal;

// current terminal in foreground
extern int curr_foreground_terminal;

// switches to a new terminal specified by target_terminal_id
void switch_terminal(int target_terminal_id);

// runs the terminal process for the specified terminal
void run_terminal_process(int target_terminal_id);

// initializes terminal_arr
void init_terminals();

#endif
