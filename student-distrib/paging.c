// paging.c - initializes page directory, page tables, and pages, as well as call assembly function to enable paging

#include "paging.h"

#include "filesystem.h"
#include "lib.h"

/*
 * page_directory_init
 *  DESCRIPTION: initializes page table directory by setting
 *      bits for each PDE entry
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: initalizes page directory
 */
void page_directory_init() {
    int i;  // for traversal

    for (i = 0; i < ENTRIES; i++) {
        page_directory[i] = DEFAULT_PDE;
    }

    // set PDEs for 4MB pages
    // memory is 4GB
    // first 4MB will be 4kB pages
    // 4MB to 4GB will be 4MB pages
    // 1023 4MB pages and 1024 4kB pages
    // 4MB pages are mapped directly from directory
    // want to 0 out last 3 bytes to make room for attricute
    // bits/only want 5 most significant bytes of address
    page_directory[0] = (((int)page_table) & ZERO_ATTRIBUTE) | TABLE_ATTRIBUTE;  // page table for 1024 4kB pages

    // global bit 1 since kernel page
    // page size bit 1 since 4MB page
    // want to cache page so 1
    // U/S is 0 since kernel is in supervisor mode
    // P is 1 since page present
    page_directory[1] = MB_OFFSET | KERNEL_PDE;  // PDE for kernel page

    page_directory[VIDMAP_PDE_IDX] = (((int)vidmap_page_table) & ZERO_ATTRIBUTE) | VIDMEM_PTE;
}

/*
 * page_table_init
 *  DESCRIPTION: initializes page table by setting default bits for
 *      each page table entry
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: initalizes page table
 */
void page_table_init() {
    int i;  // for traversal

    // memory is 4GB
    // first 4MB will be 4kB pages
    // 1000 4kB pages
    // 0x1000 is 4096 since 4096 bytes per page
    // multiply index by entry number we're on to set address
    // and OR with the attribute bits
    for (i = 0; i < KB_PAGE_COUNT; i++) {
        page_table[i] = (i * KB_OFFSET) | DEFAULT_PTE;
        vidmap_page_table[i] = (i * KB_OFFSET) | DEFAULT_PTE;
    }

    // vmem needs PCD set to 0 since we don't want to cache vmem
    // vmem is page 183
    // vmem address range: 0xB8000 - 0xB9000
    // offset of 0xB8000 bytes, so page 184
    page_table[VIDMEM_PAGE_IDX] = (VIDMEM_PAGE_IDX * KB_OFFSET) | VIDMEM_PTE;
    // 3 pages for saving screens for 3 processes
    page_table[TERMINAL_VMEM_PAGE_IDX] = (TERMINAL_VMEM_PAGE_IDX * KB_OFFSET) | VIDMEM_PTE;
    page_table[TERMINAL_VMEM_PAGE_IDX + 1] = ((TERMINAL_VMEM_PAGE_IDX + 1) * KB_OFFSET) | VIDMEM_PTE;
    page_table[TERMINAL_VMEM_PAGE_IDX + 2] = ((TERMINAL_VMEM_PAGE_IDX + 2) * KB_OFFSET) | VIDMEM_PTE;
}

/*
 * paging_init
 *  DESCRIPTION: initalize paging, including directory, table,
 *      and page setup, as well as enabling paging
 *  INPUTS: none
 *  OUTPUTS: none
 *  RETURN VALUE: none
 *  SIDE EFFECTS: enables and initializes paging
 */
void paging_init() {
    page_directory_init();           // initialize page_directory
    page_table_init();               // initialize page_table
    paging_address(page_directory);  // set cr3 to page directory address
    paging_enable();                 // enable paging
    flush_tlb();                     // flush TLB
}

/*
 * load_program
 *  DESCRIPTION: setup page at 128 MB VA for user program image
 *      and load program image into page
 *  INPUTS:
 *      command -- the command from the execute syscall
 *  OUTPUTS:
 *      prog_eip -- the prog_eip, extracted from bytes 24-27 of the program image
 *  RETURN VALUE: none
 *  SIDE EFFECTS: re-maps page at 128 MB VA to 8 MB + pid_count * 4 MB in PA, and
 *      outputs the program eip to prog_eip
 */
uint32_t load_program(const uint8_t *command, uint32_t *prog_eip) {
    /* check for garbage input values */
    if (command == NULL || prog_eip == NULL) {
        return -1;
    }
    int status;
    dentry_t dir_entry;
    /* make sure the file is an executable file */
    status = exec_file_check(command, prog_eip, &dir_entry);
    if (status == -1) {
        return -1;
    }
    /* map the user program page to be 8MB + (process number * 4MB) in physical memory */
    page_directory[USER_PROG_IDX] = (USER_PROG_PA + (MB_OFFSET * curr_pcb->process_id)) | USER_PROG_PDE;
    /* flush the TLB after swapping the page */
    flush_tlb();
    /* read the data into VA 0x8048000 */
    inode_t file_inode = filesystem.inodes[dir_entry.inode_num];
    status = read_data(dir_entry.inode_num, 0, (uint8_t *)(USER_PROG_IDX * MB_OFFSET + USER_PROG_PAGE_OFFSET), file_inode.length);
    if (status == -1) {
        return -1;
    }
    /* if the read into the userpsace VA space was successful, return 0 upon exit */
    return 0;
}
