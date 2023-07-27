#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "i8259.h"
#include "lib.h"
#include "types.h"

#define KEYBOARD_DATA_PORT 0x60

#define SCANCODES_LEN 58
// keycode found from https://www.win.tue.nl/~aeb/linux/kbd/scancodes-1.html
#define LEFT_SHIFT_PRESSED 0x2A
#define LEFT_SHIFT_RELEASED 0xAA
#define RIGHT_SHIFT_PRESSED 0x36
#define RIGHT_SHIFT_RELEASED 0xB6
#define CAPS_LOCK_PRESSED 0x3A
#define CTRL_PRESSED 0x1d
#define CTRL_RELEASED 0x9d
#define ALT_PRESSED 0x38
#define ALT_RELEASED 0xB8
#define BACKSPACE 0x0e
#define ENTER 0x1c
#define F1 0x3b
#define F2 0x3c
#define F3 0x3d

#define ESC 27

#define KEYBOARD_BUFFER_SIZE 128

#define NUM_TERMINALS 3

// buffer for storing what the user has typed
extern char keyboard_buffer[NUM_TERMINALS][KEYBOARD_BUFFER_SIZE];
extern int keyboard_buffer_head[NUM_TERMINALS];
extern int keyboard_buffer_tail[NUM_TERMINALS];
extern int length[NUM_TERMINALS];
extern volatile int enter_flag[NUM_TERMINALS];

// scancode for keyboard input
extern const char kbd_US[2 * SCANCODES_LEN];
extern const char kbd_US_CAPS[2 * SCANCODES_LEN];

void init_keyboard();

#endif
