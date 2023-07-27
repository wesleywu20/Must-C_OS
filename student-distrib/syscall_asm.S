// syscall_asm.S - system call entry point

#define ASM 1

#define ARG1 0x28 // 40 because of 36 bytes of registers + 4 bytes of return address
#define ARG2 0x2c // 44 because of 36 bytes of registers + 4 bytes of return address
#define ARG3 0x30 // 48 because of 36 bytes of registers + 4 bytes of return address

.globl open, read, write, close, halt, execute, getargs, vidmap, set_handler, sigreturn
.globl system_call_handler


jump_table:
.long 0, syscall_halt, syscall_execute, syscall_read, syscall_write, syscall_open, syscall_close, syscall_getargs, syscall_vidmap, syscall_set_handler, syscall_sigreturn

// system call has registers saved on stack already
system_call_handler:
    cmpl $0, %eax
    jle error
    cmpl $10, %eax               // 10 system calls in total
    jg error                    // invalid call number
    call *jump_table(,%eax,4)   // use jump table to find the right handler function
    iret
error:
    movl $-1, %eax              // return -1 for invalid call number
    iret

open:
    pushal // save all registers
    pushfl // save flags
    movl $5, %eax // call number for open is 5
    movl ARG1(%esp), %ebx // filename
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

read:
    pushal // save all registers
    pushfl // save flags
    movl $3, %eax // call number for read is 3
    movl ARG1(%esp), %ebx // fd
    movl ARG2(%esp), %ecx // buf
    movl ARG3(%esp), %edx // nbytes
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

write:
    pushal // save all registers
    pushfl // save flags
    movl $4, %eax // call number for write is 4
    movl ARG1(%esp), %ebx // fd
    movl ARG2(%esp), %ecx // buf
    movl ARG3(%esp), %edx // nbytes
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

close:
    pushal // save all registers
    pushfl // save flags
    movl $6, %eax // call number for close is 6
    movl ARG1(%esp), %ebx // fd
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

halt:
    pushal // save all registers
    pushfl // save flags
    movl $1, %eax // call number for halt is 1
    movl ARG1(%esp), %ebx // status
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

execute:
    pushal // save all registers
    pushfl // save flags
    movl $2, %eax // call number for execute is 2
    movl ARG1(%esp), %ebx // commands
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

getargs:
    pushal // save all registers
    pushfl // save flags
    movl $7, %eax // call number for getargs is 7
    movl ARG1(%esp), %ebx // buf
    movl ARG2(%esp), %ecx // nbytes
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

vidmap:
    pushal // save all registers
    pushfl // save flags
    movl $8, %eax // call number for vidmap is 8
    movl ARG1(%esp), %ebx // screen_start
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

set_handler:
    pushal // save all registers
    pushfl // save flags
    movl $9, %eax // call number for set_handler is 9
    movl ARG1(%esp), %ebx // signum
    movl ARG2(%esp), %ecx // handler_address
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret

sigreturn:
    pushal // save all registers
    pushfl // save flags
    movl $10, %eax // call number for sigreturn is 10
    int $0x80 // invoke a system call
    popfl // pop flags
    popl %edi // pop registers in sequence
    popl %esi
    popl %ebp
    addl $4, %esp // skip trash value
    popl %ebx
    popl %edx
    popl %ecx
    addl $4, %esp // skip eax
    ret
