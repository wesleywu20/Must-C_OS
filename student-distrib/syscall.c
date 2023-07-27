#include "syscall.h"

#include "filesystem.h"
#include "lib.h"
#include "paging.h"
#include "pcb.h"
#include "rtc.h"
#include "terminals.h"
#include "types.h"
#include "x86_desc.h"

// global variables
char arg_buf[MAX_BUF_SIZE];

// local functions
int parseCmd(uint8_t *cmd, char cmd_buf[MAX_BUF_SIZE], char arg_buf[MAX_BUF_SIZE]);
int checkFd(int fd);
uint32_t *pidToPCB(uint8_t pid);
uint32_t *pidToESP0(uint8_t pid);

/*
 * syscall_open
 *   DESCRIPTION: logic for system call open
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int syscall_open() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    // printf("eax: %u, ebx: %u, ecx: %u, edx: %u\n", call_number, arg1, arg2, arg3);
    sti();  // enable IF since int $0x80 turns it off by default
    int fd;
    if (strlen((int8_t *)arg1) == 1 && strncmp((int8_t *)arg1, ".", 1) == 0) {
        // "." is the only directory, calling dir_open
        fd = dir_open((uint8_t *)arg1);
    } else if (strlen((int8_t *)arg1) == 3 && strncmp((int8_t *)arg1, "rtc", 3) == 0) {
        fd = rtc_open((uint8_t *)arg1);
    } else {
        // everything else is a regular file
        fd = file_open((uint8_t *)arg1);
    }
    return fd;
}

/*
 * syscall_close
 *   DESCRIPTION: logic for system call close
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int syscall_close() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    // printf("eax: %u, ebx: %u, ecx: %u, edx: %u\n", call_number, arg1, arg2, arg3);
    sti();  // enable IF since int $0x80 turns it off by default
    if (checkFd(arg1) == 0) {
        // valid fd, run close function in fileops_table_t
        return ((fileops_table_t *)curr_pcb->file_desc[arg1].fileops_table_ptr)->fd_close((int32_t)arg1);
    } else {
        // invalid fd
        return -1;
    }
    // who knows what even happened if we reach here, still fail
    return -1;
}

/*
 * syscall_read
 *   DESCRIPTION: logic for system call read
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int syscall_read() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    // printf("eax: %u, ebx: %u, ecx: %u, edx: %u\n", call_number, arg1, arg2, arg3);
    sti();  // enable IF since int $0x80 turns it off by default
    if (checkFd(arg1) == 0) {
        // valid fd, run read function in fileops_table_t
        return ((fileops_table_t *)((curr_pcb->file_desc[arg1].fileops_table_ptr)))->fd_read((int32_t)arg1, (void *)arg2, (int32_t)arg3);
    } else {
        // invalid fd
        return -1;
    }
    // who even knows what happened if we reach here, still fail
    return -1;
}

/*
 * syscall_write
 *   DESCRIPTION: logic for system call write
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int syscall_write() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    // printf("eax: %u, ebx: %u, ecx: %u, edx: %u\n", call_number, arg1, arg2, arg3);
    sti();  // enable IF since int $0x80 turns it off by default
    if (checkFd(arg1) == 0) {
        // valid fd, run close function in fileops_table_t
        return ((fileops_table_t *)curr_pcb->file_desc[arg1].fileops_table_ptr)->fd_write((int32_t)arg1, (void *)arg2, (int32_t)arg3);
    } else {
        // invalid fd
        return -1;
    }
    return -1;
}

/*
 * syscall_halt
 *   DESCRIPTION: logic for system call halt
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int syscall_halt() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    // printf("eax: %u, ebx: %u, ecx: %u, edx: %u\n", call_number, arg1, arg2, arg3);
    sti();  // enable IF since int $0x80 turns it off by default

    // arg1 = status
    int i;
    unsigned int retval;
    if (arg1 == 255) {
        retval = 256;
    } else {
        retval = arg1;
    }
    // TODO: actual halt logic here
    // 1. Setup return value:
    //      a. Check if exception
    //      b. Check if program finished
    // 2. Close all processes
    for (i = 0; i < FD_SIZE; i++) {
        // REALLY UNSURE ABOUT THIS SYNTAX
        if (!checkFd(i)) {
            ((fileops_table_t *)curr_pcb->file_desc[i].fileops_table_ptr)->fd_close((int32_t)i);
        }
    }
    // 3. Set currently-active-process to non-active
    curr_pcb->active = 0;
    uint32_t parent_esp = curr_pcb->saved_esp, parent_ebp = curr_pcb->saved_ebp;
    curr_pcb->saved_ebp = 0;
    curr_pcb->saved_esp = 0;
    clear_pid(curr_pcb->process_id);
    // 4. Check if main shell
    if (curr_pcb->parent_id == -1) {
        //      a. Restart main shell
        while (1) {
            terminal_arr[curr_terminal].initialized = 0;
            execute((uint8_t *)"shell");
        }
    }
    terminal_arr[curr_terminal].curr_pcb = curr_pcb;
    point_curr_pcb(curr_pcb->parent_id);
    // 5. Not main shell handler (cntd.)
    //      a. Get parent process
    uint8_t parent_process = curr_pcb->process_id;
    //      b. Set tss for parent
    // tss.ebp = curr_pcb->saved_ebp;
    tss.esp0 = (uint32_t)pidToESP0(parent_process);
    tss.ss0 = KERNEL_DS;
    //      c. Unmap paging for current-process
    //      d. Map parent’s paging
    page_directory[USER_PROG_IDX] = (USER_PROG_PA + (MB_OFFSET * parent_process)) | USER_PROG_PDE;
    flush_tlb();
    //      e. Set parent’s process as active
    curr_pcb->active = 1;

    // 6. Halt return (asm)
    /* may need ot be in a .S file */
    asm volatile(
        "               \n\
        movl %0, %%ebp  \n\
        movl %1, %%esp  \n\
        movl %2, %%eax  \n\
        leave           \n\
        ret             \n\
        "
        :
        : "r"(parent_ebp), "r"(parent_esp), "r"(retval)
        : "memory", "cc");

    // // should never reach here
    return -1;
}

/*
 * syscall_execute
 *   DESCRIPTION: logic for system call execute
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int syscall_execute() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    // printf("eax: %u, ebx: %u, ecx: %u, edx: %u\n", call_number, arg1, arg2, arg3);
    sti();  // enable IF since int $0x80 turns it off by default

    // 1. paging helpers (optional but recommended)
    //   - map virtual & physical memory
    //   - unmap virtual & physical memory
    // 2. parse cmd
    char cmd_buf[MAX_BUF_SIZE];
    // make sure the buffer doesn't have any garbage that can affect the result
    memset(cmd_buf, '\0', MAX_BUF_SIZE);
    memset(arg_buf, '\0', MAX_BUF_SIZE);
    if (parseCmd((uint8_t *)arg1, cmd_buf, arg_buf) != 0) {
        return -1;
    }
    // 3. file checks
    dentry_t trash_dir_entry;
    int trash_int;
    int status = exec_file_check((uint8_t *)cmd_buf, (uint32_t *)&trash_int, &trash_dir_entry);
    if (status == -1) {
        return -1;
    }
    // 7. setup old stack & eip
    register uint32_t saved_ebp asm("ebp");
    register uint32_t saved_esp asm("esp");
    // 4. create new PCB
    int available_pid = find_avail_pid();
    if (available_pid == -1) {
        // printf("No available space in PCB\n");
        // point_curr_pcb(5);  // TODO: sus code here
        return -1;
    }
    int parentID = terminal_arr[curr_terminal].initialized == 0 ? -1 : curr_pcb->process_id;
    terminal_arr[curr_terminal].initialized = 1;
    int currID = create_pcb(parentID, saved_ebp, saved_esp);
    point_curr_pcb(currID);
    // 5. setup memory/paging
    // 6. read exe data
    uint32_t prog_eip;
    status = load_program((uint8_t *)cmd_buf, &prog_eip);
    if (status == -1) {
        return -1;
    }
    curr_pcb->saved_eip = prog_eip;
    tss.esp0 = (uint32_t)pidToESP0(currID);
    tss.ss0 = KERNEL_DS;
    // 8. goto usermode
    cli();

    // 43 = 0x2B = USER_DS
    // 35 = 0x23 = USER_CS
    asm volatile(
        "                            \n\
            movw    $43, %%ax      \n\
            movw    %%ax, %%ds       \n\
            movw    %%ax, %%es       \n\
            movw    %%ax, %%fs       \n\
            movw    %%ax, %%gs       \n\
                                     \n\
            pushl   $43            \n\
            pushl   %%edx            \n\
            pushf                    \n\
            pushl   $35            \n\
            pushl   %%ecx        \n\
            iret                     \n\
            "
        :
        : "c"(prog_eip), "d"(0x08400000)
        : "memory", "cc");
    return 0;
}

/*
 * syscall_getargs
 *   DESCRIPTION: logic for system call getargs
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t syscall_getargs() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    sti();                                                                  // enable IF since int $0x80 turns it off by default
    // arg1 = uint8_t* buf
    // arg2 = int32_t nbytes
    if (arg1 == NULL || arg_buf == NULL || arg_buf[0] == '\0') {
        // check if either buffer is null or if there are no arguments
        return -1;
    }
    memcpy((void *)arg1, (void *)arg_buf, (uint32_t)arg2);  // copy arg_buf to buf
    return 0;
}

/*
 * syscall_vidmap
 *   DESCRIPTION: logic for system call vidmap
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t syscall_vidmap() {
    unsigned int call_number, arg1, arg2, arg3;
    asm volatile(""
                 : "=a"(call_number), "=b"(arg1), "=c"(arg2), "=d"(arg3));  // retrieve register values
    sti();                                                                  // enable IF since int $0x80 turns it off by default
    /* check that the address passed in is within the 4 MB user program page */
    if (arg1 < USER_PROG_IDX * MB_OFFSET || arg1 >= VIDMAP_PDE_IDX * MB_OFFSET || (uint8_t **)arg1 == NULL) {
        return -1;
    }
    /* map page in vidmap page table into the PA of video memory */
    vidmap_page_table[curr_pcb->process_id] = (VIDMEM_PAGE_IDX * KB_OFFSET) | VIDMEM_PTE;
    /* get address of page just mapped to video memory */
    uint8_t *vidmap_addr = (uint8_t *)(((VIDMAP_PDE_IDX << VIDMAP_PDE_IDX_POS) | (curr_pcb->process_id << VIDMAP_PTE_IDX_POS)) & ZERO_ATTRIBUTE);
    /* map pointer from screen_start argument to that address */
    *((uint8_t **)arg1) = vidmap_addr;
    /* return 0 for success */
    return 0;
}

/*
 * syscall_set_handler
 *   DESCRIPTION: logic for system call set_handler
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t syscall_set_handler() {
    return -1;
}

/*
 * syscall_sigreturn
 *   DESCRIPTION: logic for system call sigreturn
 *   INPUTS: arguments in registers from eax to edx
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
int32_t syscall_sigreturn() {
    return -1;
}

/*
 * parseCmd
 *   DESCRIPTION: parse cmd into command and arguments
 *   INPUTS: cmd - string to be parsed
 *   OUTPUTS: command in cmd_buf, arguments in arg_buf
 *   RETURN VALUE: 0 for successful parsing
 *   SIDE EFFECTS: none
 */
int parseCmd(uint8_t *cmd, char cmd_buf[MAX_BUF_SIZE], char arg_buf[MAX_BUF_SIZE]) {
    int start = 0, curr = 0, length = 0;  // start and curr records start and end index of the command, length is the length of it
    int curr_cmd = 0;                     // current argument index
    char *currBuf = cmd_buf;              // curr buffer to write to
    while (cmd[curr] != '\0') {           // iterate through the null-terminated string
        if (cmd[curr] == ' ' && curr_cmd == 0) {
            // parse by space
            memcpy(currBuf, cmd + start, length);
            // update next buffer to write
            if (currBuf == cmd_buf) {
                currBuf = arg_buf;
                curr_cmd = 1;
            } /*else {
                curr_arg += 1;
                currBuf = arg_buf[curr_arg];
            }*/
            // update length and start index of next argument
            length = -1;       // -1 because we are adding 1 below
            start = curr + 1;  // move start past space
        }
        // update curr and length
        curr += 1;
        length += 1;
    }
    memcpy(currBuf, cmd + start, length);
    return 0;
}

/*
 * checkFd
 *   DESCRIPTION: helpful to check if index fd of PCB is valid
 *   INPUTS: fd - index of PCB to check
 *   OUTPUTS: none
 *   RETURN VALUE: 0 - file exists, valid fd
 *                 -1 - file doesn't exist, invalid fd
 *   SIDE EFFECTS: none
 */
int checkFd(int fd) {
    if (fd < 0 || fd >= PCB_SIZE || !curr_pcb->file_desc[fd].flags) {
        // invalid fd
        return -1;
    } else {
        // valid fd
        return 0;
    }
}

/*
 * pidToESP0
 *   DESCRIPTION: a helper to find esp0 for a pid
 *   INPUTS: pid - process id number
 *   OUTPUTS: none
 *   RETURN VALUE: address of esp0 for that pid
 *   SIDE EFFECTS: none
 */
uint32_t *pidToESP0(uint8_t pid) {
    return (uint32_t *)(KP_BOTTOM - KS_SIZE * (pid));
}
