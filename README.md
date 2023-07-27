# A Must-C OS
As part of an 8-week long final project for our OS class, ECE 391, I, along with teammates Bobby Zhou, Isha Pendem, and Leo Schopick, created the core of a Linux-like OS kernel.

## Features
The kernel implements:

- Setup of the interrupt descriptor table (IDT), along with support for hardware interrupts for up to 16 devices
- A keyboard driver, with shortcuts for clearing the terminal (ctrl + l) and switching between terminals (Alt + F1/F2/F3)
- RTC: the processing of periodic interrupts from the real-time clock (a virtual MC146818A chip)
- Multiple terminals, with the ability to execute different programs within each terminal
- an ext2fs filesystem with support for execution of six total tasks from program images within the filesystem 
- paging for conversion of virtual memory addresses to physical memory addresses
- ten system call interfaces, including execution and halting of programs

## Implementation

The kernel is implemented mostly in C with some x86 assembly language where needed, and is loaded onto a QEMU virtual machine using the GRUB bootloader.

## Challenges

All of the challenges we faced while implementing this OS kernel are outlined in student-distrib\buglog.txt, along with how we resolved each one.

## Acknowledgements

I would like to take a moment to acknowledge all the ECE 391 CAs/TAs who've helped our team along the way, as well as my teammates for all making substantial contributions to the development of this kernel.