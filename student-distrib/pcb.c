#include "pcb.h"
#include "lib.h"
#include "filesystem.h"
#include "types.h"

// // kernel stack base addresses
// // 0x007FFFFF: bottom of kernel stack 1 at 8MB-1
// uint32_t KS_1 = KS1; 
// // 0x007FDFFF: bottom of kernel stack 2 at 8MB-8kB-1
// uint32_t KS_2 = KS2;

pcb_t* pcb_arr[MAX_PROC];
pcb_t *curr_pcb = (pcb_t *)PCB1_POS;

/*
 * point_curr_pcb
 *   DESCRIPTION: points curr_pcb to the pcb number inputted 
 *   INPUTS: 
 *      pcbx -- pcb process to be pointed to
 *   OUTPUTS: none
 *   RETURN VALUE: pcb pointed to if success, -1 if failure
 *   SIDE EFFECTS: points curr_pcb ptr to indicated pcb
 */

int point_curr_pcb(int pcbx) {
    if (pcbx >= 0 && pcbx < MAX_PROC) {
        curr_pcb = pcb_arr[pcbx];
        return pcbx;
    } else {
        return 0;
    }
    return -1; // return -1 on failure
}

/*
 * find_avail_pid
 *   DESCRIPTION: finds pid available in pid_arr 
 *   INPUTS: NONE
 *   OUTPUTS: none
 *   RETURN VALUE: first available pid for success, 
 *      -1 for failure
 *   SIDE EFFECTS: NONE
 */
int find_avail_pid() {
    int i; // traverse
    for(i = 0; i < MAX_PROC; i++) { // go through each entry in pid_arr
        if(pcb_arr[i]->available == 1) { // check if its available
            pcb_arr[i]->process_id = i;  // set the process id of the pid_arr entry
            return i; // return index that's available
        }
    }
    return -1; // if neither are empty
}

/*
 * clear_pid
 *   DESCRIPTION: clears the pid in pid_arr after done 
 *   INPUTS: 
 *      pid -- pid we want to clear
 *   OUTPUTS: none
 *   RETURN VALUE: pid num cleared if success, -1 for failure
 *   SIDE EFFECTS: sets specified pid to available in pid_arr
 */
int clear_pid(int pid) {
    if ((pid >= 0) && (pid < MAX_PROC)) {  // for each entry
        pcb_arr[pid]->available = 1; // set as available
        return pid; // return pid cleared
    }
    return -1; // return for failure
}

/*
 * pcb_arr_init
 *   DESCRIPTION: initializes pcb_arr so that all entries are 
 *      marked as available 
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   RETURN VALUE: NONE
 *   SIDE EFFECTS: sets all entries to available
 */
void pcb_arr_init() {
    int i; // for traversal
    for (i = 0; i < MAX_PROC; i++) {  // for each entry
        pcb_arr[i] = (pcb_t *)pidToPCB(i);
        memset((void *)pcb_arr[i], 0, sizeof(pcb_t));
        start_process(pcb_arr[i]->file_desc);
        pcb_arr[i]->available = 1;  // set as available
    }
    return;
}


/*
 * create_pcb
 *   DESCRIPTION: creates PCB to place in kernel 
 *      space during execute 
 *   INPUTS: 
 *      parentID -- process ID of program that called 
 *          it
 *      curr_ebp -- current ebp of execute so it can be 
 *          returned to after syscall execution completed
 *      curr_esp -- current esp of execute before syscall 
 *          is executed
 *   OUTPUTS: none
 *   RETURN VALUE: pid of process created
 *   SIDE EFFECTS: fills in PCB1 at top of kernel stack
 */
int create_pcb(int parentID, uint32_t curr_ebp, uint32_t curr_esp) {
    int i; // to store pcb_arr index PCB process ID will be placed in
    i = find_avail_pid(); // find available entry in pcb_arr
    pcb_arr[i]->available = 0; // set unavailable
    // set PCB attributes
    pcb_arr[i]->process_id = i;
    pcb_arr[i]->parent_id = parentID;
    pcb_arr[i]->saved_esp = curr_esp;
    pcb_arr[i]->saved_ebp = curr_ebp;
    pcb_arr[i]->active = 1;
    return i;
}

/*
 * pidToPCB
 *   DESCRIPTION: a helper to find start of PCB for a pid
 *   INPUTS: pid - process id number
 *   OUTPUTS: none
 *   RETURN VALUE: address of start of PCB for that pid
 *   SIDE EFFECTS: none
 */ 
uint32_t* pidToPCB(uint8_t pid){
    return (uint32_t*)(KP_BOTTOM - KS_SIZE * (pid + 1) + PCB_SIZE);
}
