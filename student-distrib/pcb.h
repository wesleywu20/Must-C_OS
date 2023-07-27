#ifndef _PCB_H
#define _PCB_H

#include "types.h"

#define KS_SIZE 8192                                         // each kernel stack is 8192B or 8kB
#define KP_BOTTOM 8388608                                    // kernel page ends at 8MB
#define KS1 (KP_BOTTOM - 1)                                  // kernel stack 1 grows from 8MB to lower addresses (exclusive so -1)
#define KS2 (KP_BOTTOM - KS_SIZE - 1)                        // kernel stack 2 grows from 8MB-8kB to lower addresses (exclusive so -1)
#define FD_SIZE 8                                            // each fd can open max 8 files, including stdin and stdout
#define PCB_SIZE 256                                         // size of PCB is bytes, aligned to 4
#define PCB1_POS (KP_BOTTOM - KS_SIZE + PCB_SIZE)            // finding starting addr of PCB1
#define PCB2_POS (KP_BOTTOM - KS_SIZE - KS_SIZE + PCB_SIZE)  // finding starting addr of PCB2
#define MAX_PROC 6                                           // max number of processes we can have

/*
 * struct for file_descriptor with file descriptor attributes
 */
typedef struct file_descriptor {
    int32_t* fileops_table_ptr;
    int32_t inode;
    int32_t file_pos;
    int32_t flags;
} file_descriptor_t;

/*
 * struct holding all the PCB attributes
 */
typedef struct pcb {
    int process_id;
    int parent_id;
    file_descriptor_t file_desc[FD_SIZE];
    uint32_t saved_esp;
    uint32_t saved_ebp;
    uint32_t saved_eip;
    uint8_t active;
    uint8_t available;
} pcb_t;

/* global array of PIDs to be able to assign PCBs process IDs and
keep track of nonactive processes without needing to store the
whole PCB */
extern pcb_t* pcb_arr[MAX_PROC];

/* global pointer that points to the current PCB we're
using/active PCB */
extern pcb_t* curr_pcb;

// called in create_pcb_x
int find_avail_pid();

// need to be called when we are done with the process
int clear_pid(int pid);

// point to other PCB
int point_curr_pcb(int pcbx);

// initialize pid array
void pcb_arr_init();

// create_pcb_x
int create_pcb(int parentID, uint32_t curr_ebp, uint32_t curr_esp);

// pidToPCB
uint32_t* pidToPCB(uint8_t pid);

#endif
