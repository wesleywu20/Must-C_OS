# paging.S - assembly for paging

#define ASM     1
##include "x86_desc.h"

.text

.globl paging_enable, paging_address, flush_tlb

.align 4                    # align on 4 byte address

paging_enable:
    pushl %ebp              # callee sequence
    movl %esp, %ebp
    pushl %ebx              # save callee saved regs
    pushl %esi
    pushl %edi

    mov %cr4, %eax         # store cr4 in intermediate reg
    orl $0x10, %eax         # set the PSE bit (bit 4) to allow multiple sized paging (4MB and 4kB)
    andl $0xFFFFFFDF, %eax  # make sure PAE bit is 0 so larger page size is 4 MB and not 2 MB
    mov %eax, %cr4         # store back into cr4

    movl %cr0, %eax         # store cr0 in intermediate reg for manipulation
    orl $0x80000001, %eax   # enable paging (PG (bit 31) and PE (bit 0)), everything else 0
    movl %eax, %cr0         # store cr0 in intermediate reg for manipulation

    popl %edi               # pop callee saved regs
    popl %esi
    popl %ebx
    leave
    ret 

paging_address:
    pushl %ebp              # callee sequence
    movl %esp, %ebp
    pushl %ebx              # save callee saved regs
    pushl %esi
    pushl %edi

    movl 8(%ebp), %eax      # put arg1 page_directory address into eax
    mov %eax, %cr3         # put address into cr3

    popl %edi               # pop callee saved regs
    popl %esi
    popl %ebx
    leave                   
    ret

flush_tlb:
    pushl %ebp              # callee sequence
    movl %esp, %ebp
    pushl %ebx              # save callee saved regs
    pushl %esi
    pushl %edi

    mov %cr3, %eax         # touch cr3 to invalidate address and flush tlb
    mov %eax, %cr3

    popl %edi               # pop callee saved regs
    popl %esi
    popl %ebx
    leave                   
    ret
