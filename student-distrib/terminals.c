#include "terminals.h"

#include "lib.h"
#include "paging.h"
#include "pcb.h"
#include "syscall.h"
#include "x86_desc.h"

// global variables
terminal_t terminal_arr[NUM_TERMINALS];
int curr_terminal = 0;
int curr_foreground_terminal = 0;

// local functions
char* get_page_addr_from_terminal_id(int terminal_id);
void save_video_mem(char* vmem, int terminal_id);
void restore_video_mem(char* vmem, int terminal_id);

/*
 * init_terminals
 *   DESCRIPTION: initializes the terminal array
 *   INPUTS: none
 *   OUTPUTS: terminal_arr initialized with 0s
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_terminals() {
    int i;
    for (i = 0; i < NUM_TERMINALS; i++) {
        memset(&terminal_arr[i], 0, sizeof(terminal_t));
    }
}

/*
 * function_name: save_screen
 *   DESCRIPTION: saves the current video memory and cursor position
 *   INPUTS: vmem -- pointer to video memory
 *   OUTPUTS: current screen saved to process_vmem[curr_pid],
 *            cursor position saved to processes[curr_process]
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void save_video_mem(char* vmem, int terminal_id) {
    int i;  // loop counter
    char* terminal_vmem_addr = get_page_addr_from_terminal_id(terminal_id);
    for (i = 0; i < NUM_COLS * NUM_ROWS; i++) {
        terminal_vmem_addr[i] = vmem[i << 1];
    }
    terminal_arr[curr_terminal].cursor_x = screen_x;
    terminal_arr[curr_terminal].cursor_y = screen_y;
}

/*
 * restore_video_mem
 *   DESCRIPTION: restores the video memory and cursor position from previously saved page
 *   INPUTS: vmem -- pointer to video memory
 *   OUTPUTS: screen restored to previously saved screen,
 *            cursor position restored as well
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void restore_video_mem(char* vmem, int terminal_id) {
    int i;  // loop counter
    char* terminal_vmem_addr = get_page_addr_from_terminal_id(terminal_id);
    for (i = 0; i < NUM_COLS * NUM_ROWS; i++) {
        vmem[i << 1] = terminal_vmem_addr[i];
    }
    screen_x = terminal_arr[terminal_id].cursor_x;
    screen_y = terminal_arr[terminal_id].cursor_y;
    update_cursor(screen_x, screen_y);
}

/*
 * get_page_addr_from_terminal_id
 *   DESCRIPTION: gets the address of page to store a terminal's video memory
 *   INPUTS: terminal_id -- id of terminal to get address of
 *   OUTPUTS: none
 *   RETURN VALUE: address of page to store a terminal's video memory
 *   SIDE EFFECTS: none
 */
char* get_page_addr_from_terminal_id(int terminal_id) {
    if (terminal_id < 0 || terminal_id >= NUM_TERMINALS) {
        return NULL;
    }
    return (char*)((terminal_id + TERMINAL_VMEM_PAGE_IDX) * KB_OFFSET);
}

/*
 * switch_terminal
 *   DESCRIPTION: switches to a terminal specified by target_terminal_id
 *   INPUTS: terminal_id -- id of terminal to switch to
 *   OUTPUTS: terminal switched to the one with target_terminal_id
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
void switch_terminal(int target_terminal_id) {
    // check if target_terminal_id is valid
    if (target_terminal_id < 0 || target_terminal_id >= NUM_TERMINALS) {
        return;
    }
    // don't switch if already on target terminal
    if (target_terminal_id == curr_foreground_terminal) {
        return;
    }
    // save current terminal's information
    char* vmem_addr = (char*)VIDEO;
    save_video_mem(vmem_addr, curr_foreground_terminal);  // save current screen
    restore_video_mem(vmem_addr, target_terminal_id);     // load target terminal's screen
    curr_foreground_terminal = target_terminal_id;
    run_terminal_process(target_terminal_id);  // run the process in the new terminal
}

/*
 * run_terminal_process
 *   DESCRIPTION: run the process in the terminal specified by target_terminal_id
 *   INPUTS: terminal_id -- id of terminal to switch to
 *   OUTPUTS: terminal switched to the one with target_terminal_id
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: none
 */
void run_terminal_process(int target_terminal_id) {
    // check if target_terminal_id is valid
    if (target_terminal_id < 0 || target_terminal_id >= NUM_TERMINALS) {
        return;
    }
    // don't need to restore the video memory because processes can run in the background
    // save current terminal's stack information
    register uint32_t curr_ebp asm("ebp");
    register uint32_t curr_esp asm("esp");
    terminal_arr[curr_terminal].ebp = curr_ebp;
    terminal_arr[curr_terminal].esp = curr_esp;
    terminal_arr[curr_terminal].esp0 = tss.esp0;
    terminal_arr[curr_terminal].ss0 = tss.ss0;
    terminal_arr[curr_terminal].curr_pcb = curr_pcb;
    curr_terminal = target_terminal_id;  // point to new terminal
    // if target terminal is not initialized, initialize it
    if (terminal_arr[target_terminal_id].initialized == 0) {
        sti();
        execute((uint8_t*)"shell");
    }
    /* map the user program page to be 8MB + (process number * 4MB) in physical memory */
    pcb_t* target_terminal_pcb = terminal_arr[target_terminal_id].curr_pcb;
    curr_pcb = target_terminal_pcb;
    page_directory[USER_PROG_IDX] = (USER_PROG_PA + (MB_OFFSET * target_terminal_pcb->process_id)) | USER_PROG_PDE;
    /* flush the TLB after swapping the page */
    flush_tlb();

    // use return and iret to jump to target terminal
    tss.esp0 = terminal_arr[target_terminal_id].esp0;
    tss.ss0 = terminal_arr[target_terminal_id].ss0;
    uint32_t target_esp = terminal_arr[target_terminal_id].esp;
    uint32_t target_ebp = terminal_arr[target_terminal_id].ebp;
    sti();

    asm volatile(
        "                            \n\
            movl %0, %%ebp           \n\
            movl %1, %%esp           \n\
            "
        :
        : "r"(target_ebp), "r"(target_esp)
        : "memory", "cc");
}
