#ifndef PAGING_H
#define PAGING_H

// #include <stdint.h>
#include "types.h"
// #include "paging_asm.S"

// oh its just like magic! - magic numbers!
#define ENTRIES 1024
#define BYTES_PER_ENTRY 4
#define SIZE (ENTRIES * BYTES_PER_ENTRY)
#define MB_PAGE_COUNT 1023
#define KB_PAGE_COUNT 1024
#define KB_OFFSET 0x1000   /* 4kB */
#define MB_OFFSET 0x400000 /* 4MB */
/*
 * DEFAULT_PDE
 * Page Base Addr[31:22] | Reserved[21:13] | PAT[12] | Available[11:9] | G[8] | PS[7] | D[6] | A[5] | PCD[4] | PWT[3] | U/S[2] | R/W[1] | P[0]
 * 0000000000              000000000         0         000               0      1       0      0      0        0        0        1        0
 * default - all pages are R/W
 * do not mark present at beginning
 * for now, mark all pages 4MB with PS bit, since only one entry in directory will point to 4kB page
 */
#define DEFAULT_PDE 0x00000082

/*
 * TABLE_ATTRIBUTE
 * Page Base Addr[31:12] | Available[11:9] | G[8] | PS[7] | 0 | A[5] | PCD[4] | PWT[3] | U/S[2] | R/W[1] | P[0]
 * 00000000000000000000    000               0      0       0   0      1        0        0        1        1
 * default - all pages are R/W
 * page table will be present
 * do not mark directory entry for page table as 4MB (bit 7)
 * want to use cache (PCD set to 1)
 */
#define TABLE_ATTRIBUTE 0x00000013

// want to zero out last 3 bytes to make room for attribute bits in entry
#define ZERO_ATTRIBUTE 0xFFFFF000

/*
 * KERNEL_PDE
 * Page Base Addr[31:22] | Reserved[21:13] | PAT[12] | Available[11:9] | G[8] | PS[7] | D[6] | A[5] | PCD[4] | PWT[3] | U/S[2] | R/W[1] | P[0]
 * 0000000000              000000000         0         000               1      1       0      0      1        0        0        1        1
 * default - all pages are R/W
 * mark page as present
 * want to use cache
 */
#define KERNEL_PDE 0x00000193

/*
 * DEFAULT_PTE
 * Page Base Addr[31:12] | Available[11:9] | G[8] | PAT[7] | D[6] | A[5] | PCD[4] | PWT[3] | U/S[2] | R/W[1] | P[0]
 * 00000000000000000000    000               0      0        0      0      1        0        0        1        1
 * default - all pages are R/W
 * all pages in page table will be present
 * do not mark directory entry for page table as 4MB (bit 7)
 * want to use cache (PCD set to 1)
 */
#define DEFAULT_PTE 0x00000016

/*
 * VIDMEM_PTE
 * Page Base Addr[31:12] | Available[11:9] | G[8] | PAT[7] | D[6] | A[5] | PCD[4] | PWT[3] | U/S[2] | R/W[1] | P[0]
 * 00000000000000000000    000               0      0        0      0      0        0        1        1        1
 * default - all pages are R/W
 * do not mark directory entry for page table as 4MB (bit 7)
 * do not want to use cache for vidmem (PCD set to 0)
 */
#define VIDMEM_PTE 0x00000007

// user programs loaded into page at 128 MB, and since each page is 4 MB, this will be the 32nd page in the directory
#define USER_PROG_IDX 32

// vidmap page directory page table index
#define VIDMAP_PDE_IDX (USER_PROG_IDX + 1)

// vidmap page directory page table index position in 32-bit addr - 22 left shifts to address[31:22]
#define VIDMAP_PDE_IDX_POS 22

// vidmap page table entry index
#define VIDMAP_PTE_IDX 420

// vidmap page directory page table index position in 32-bit addr - 12 left shifts to address[21:12]
#define VIDMAP_PTE_IDX_POS 12

// user programs loaded into PA 8 MB (right after the kernel)
#define USER_PROG_PA (2 * MB_OFFSET)

// user program page offset
#define USER_PROG_PAGE_OFFSET 0x48000

/*
 * USER_PROG_PDE
 * Page Base Addr[31:22] | Reserved[21:13] | PAT[12] | Available[11:9] | G[8] | PS[7] | D[6] | A[5] | PCD[4] | PWT[3] | U/S[2] | R/W[1] | P[0]
 * 0000000000              000000000         0         000               1      1       0      0      1        0        0        1        1
 * default - all pages are R/W
 * mark page as present
 * want to use cache
 */
#define USER_PROG_PDE 0x00000197

// vidmem starts at address 0xB8000, so vidmem is 184th page in page table
#define VIDMEM_PAGE_IDX 184

// starting page index for storing video memroy for terminal 1 through 3
#define TERMINAL_VMEM_PAGE_IDX 69

// page directory
extern uint32_t page_directory[ENTRIES] __attribute__((aligned(SIZE)));

// page table
extern uint32_t page_table[ENTRIES] __attribute__((aligned(SIZE)));

// vidmap page table
extern uint32_t vidmap_page_table[ENTRIES] __attribute__((aligned(SIZE)));

// initializes paging, including enable, directory and page setup
void paging_init();

// assembly - paging enable function
extern void paging_enable();

// initializing page directory
void page_directory_init();

// initializing page table
void page_table_init();

// loading a progarm into memory
uint32_t load_program(const uint8_t* command, uint32_t* prog_eip);

// assembly - put page directory address into cr3
extern void paging_address(unsigned int*);

// assembly - flush tlb
extern void flush_tlb();

#endif /* PAGING_H */
