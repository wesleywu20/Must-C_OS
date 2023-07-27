#include "keyboard.h"

#include "terminal.h"
#include "terminals.h"

// local functions
void _keyboard_interrupt_handler();
void add_to_end_of_buffer(char newChar);
char update_keyboard_buffer(char key);
void send_to_screen(char key, char lastChar);

// local variables
static int shift = 0;
static int caps_lock = 0;
static int ctrl = 0;
static int alt = 0;
static int char_counter[NUM_TERMINALS] = {0, 0, 0};

// initalize global variables
char keyboard_buffer[NUM_TERMINALS][KEYBOARD_BUFFER_SIZE] = {{0}};
int keyboard_buffer_head[NUM_TERMINALS] = {0};  // index of first character, inclusive
int keyboard_buffer_tail[NUM_TERMINALS] = {0};  // index of last character + 1
int length[NUM_TERMINALS] = {0};
volatile int enter_flag[NUM_TERMINALS] = {0};  // whether enter has been pressed

// scancode to ascii conversion from https://stackoverflow.com/questions/61124564/convert-scancodes-to-ascii
const char kbd_US[2 * SCANCODES_LEN] =
    {
        // standard (non-shifted) characters
        0, ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', /* <-- Tab */
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, /* <-- control key */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*',
        0,   /* Alt */
        ' ', /* Space bar */

        // shifted characters
        0, ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', /* <-- Tab */
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
        0, /* <-- control key */
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
        '*',
        0,  /* Alt */
        ' ' /* Space bar */
};
// another version of scancode when CAPS is on
const char kbd_US_CAPS[2 * SCANCODES_LEN] =
    {
        // standard (non-shifted) characters
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', /* <-- Tab */
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',
        0, /* <-- control key */
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0,
        '*',
        0,   /* Alt */
        ' ', /* Space bar */

        // shifted characters
        0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
        '\t', /* <-- Tab */
        'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}', '\n',
        0, /* <-- control key */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '\"', '~', 0, '|', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?', 0,
        '*',
        0,  /* Alt */
        ' ' /* Space bar */
};

/*
 * init_keyboard
 *   DESCRIPTION: initializes keyboard
 *   INPUTS: none
 *   OUTPUTS: the keyboard is initialized and the shift parameter is set to off by default
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void init_keyboard() {
    shift = 0;  // shift defaults to 0
    caps_lock = 0;
    alt = 0;
    ctrl = 0;
    enable_irq(KEYBOARD_IRQ_NUM);
}

/*
 * _keyboard_interrupt_handler
 *   DESCRIPTION: handler for keyboard which is passed into assembly linkage in idt_linkage.S
 *                executed in a critical section
 *   INPUTS: none
 *   OUTPUTS: the key pressed will be printed to the screen
 *   RETURN VALUE: none
 *   SIDE EFFECTS: other interrupt handlers cannot run simultaneously because of critical section
 */
void _keyboard_interrupt_handler() {
    cli();
    uint32_t key = inb(KEYBOARD_DATA_PORT);  // grab input from the data port
    switch (key) {
        // set modifier correctly (ctrl, alt, shift, caps_lock)
        // for shift, ctrl, and alt,
        // pressed would turn on (1) the modifier
        // released would turn off (0) the modifier
        case LEFT_SHIFT_PRESSED:
        case RIGHT_SHIFT_PRESSED:
            shift = 1;
            break;
        case LEFT_SHIFT_RELEASED:
        case RIGHT_SHIFT_RELEASED:
            shift = 0;
            break;
        case CAPS_LOCK_PRESSED:
            // caps lock is toggled each time it's pressed
            caps_lock = ~caps_lock;
            break;
        case CTRL_PRESSED:
            ctrl = 1;
            break;
        case CTRL_RELEASED:
            ctrl = 0;
            break;
        case ALT_PRESSED:
            alt = 1;
            break;
        case ALT_RELEASED:
            alt = 0;
            break;
        case F1:
            if (alt && !ctrl) {
                // switch to terminal 0
                send_eoi(KEYBOARD_IRQ_NUM);
                switch_terminal(0);
            }
            break;
        case F2:
            if (alt && !ctrl) {
                // switch to terminal 1
                send_eoi(KEYBOARD_IRQ_NUM);
                switch_terminal(1);
            }
            break;
        case F3:
            if (alt && !ctrl) {
                // switch to terminal 2
                send_eoi(KEYBOARD_IRQ_NUM);
                switch_terminal(2);
            }
            break;
        default:
            // check if valid scan code
            if (key >= 0 && key < SCANCODES_LEN) {
                // update the keyboard buffer and send key to screen
                // while keeping track of last character deleted if backspace
                char lastChar = update_keyboard_buffer(key);
                send_to_screen(key, lastChar);
            }
            break;
    }

    // send eoi to IRQ1, the port that keyboard occupies on PIC
    send_eoi(KEYBOARD_IRQ_NUM);
    // end critical section
    sti();
}

/*
 * update_keyboard_buffer
 *   DESCRIPTION: update the keyboard buffer based on the key pressed
 *   INPUTS: key - key pressed
 *   OUTPUT: keyboard buffer updated based on key press
 *   RETURN VALUE: if backspace is pressed, the character removed from keyboard buffer is returned
 *                 otherwise the newly inserted character is returned
 *   SIDE EFFECTS: update length, keyboard_buffer_head, keyboard_buffer_tail if necessary
 */
char update_keyboard_buffer(char key) {
    // grab the ascii value of keypress
    char to_screen = caps_lock == 0
                         ? kbd_US[key + shift * SCANCODES_LEN]
                         : kbd_US_CAPS[key + shift * SCANCODES_LEN];
    if (enter_flag[curr_foreground_terminal]) {                                 // if the enter has been pressed previously
        keyboard_buffer_head[curr_foreground_terminal] = keyboard_buffer_tail[curr_foreground_terminal];  // move head to start a new line
        enter_flag[curr_foreground_terminal] = 0;                               // reset flag to 0
        length[curr_foreground_terminal] = 0;                                   // new line has length 0
    }
    if (key == BACKSPACE) {
        // move the pointer back by 1
        keyboard_buffer_tail[curr_foreground_terminal] = keyboard_buffer_tail[curr_foreground_terminal] > keyboard_buffer_head[curr_foreground_terminal]
                                   ? keyboard_buffer_tail[curr_foreground_terminal] - 1
                                   : keyboard_buffer_head[curr_foreground_terminal];
        char lastChar = keyboard_buffer[curr_foreground_terminal][keyboard_buffer_tail[curr_foreground_terminal]];  // save the char to be deleted
        keyboard_buffer[curr_foreground_terminal][keyboard_buffer_tail[curr_foreground_terminal]] = 0;              // remove the last character in this line
        length[curr_foreground_terminal] = max(length[curr_foreground_terminal] - 1, 0);                            // update length
        return lastChar;
    } else {
        if (length[curr_foreground_terminal] < KEYBOARD_BUFFER_SIZE) {
            if (ctrl == 1 || alt == 1) {
                // don't update buffer for ctrl+any key or alt+any key
                return 0;
            }
            // only update buffer and increments tail when buffer has below 128 elements
            keyboard_buffer[curr_foreground_terminal][keyboard_buffer_tail[curr_foreground_terminal]] = to_screen;                         // insert character into buffer
            keyboard_buffer_tail[curr_foreground_terminal] = (keyboard_buffer_tail[curr_foreground_terminal] + 1) % KEYBOARD_BUFFER_SIZE;  // move tail forward by 1
            length[curr_foreground_terminal] += 1;                                                               // increment length
            return to_screen;
        }
    }
    return 0;
}

void send_to_screen(char key, char lastChar) {
    // find the corresponding scancode
    char to_screen = caps_lock == 0 ? kbd_US[key + shift * SCANCODES_LEN] : kbd_US_CAPS[key + shift * SCANCODES_LEN];
    if (ctrl == 1 && (to_screen == 'l' || to_screen == 'L')) {  // clear screen for C-L and C-l pressed
        clear();
    } else if (ctrl == 0 && alt == 0) {  // disable output if any modifier key pressed
        switch (to_screen) {
            case 0:
            case ESC:
                // don't print null or weird character
                break;
            case '\b':
                if (lastChar == '\t') {
                    int i;  // loop index
                    for (i = 0; i < 4; i++) {
                        // remove 4 spaces if last character is tab
                        terminal_write(0, &to_screen, 1);
                    }
                    char_counter[curr_foreground_terminal] -= 4;
                } else if (char_counter[curr_foreground_terminal] > 0) {
                    // send the ascii and let terminal driver decide what to do
                    terminal_write(0, &to_screen, 1);
                    char_counter[curr_foreground_terminal]--;
                }
                break;
            case '\n':
                enter_flag[curr_foreground_terminal] = 1;  // set enter_flag to 1 to indicate enter has been pressed
                char_counter[curr_foreground_terminal] = -1;
            default:
                terminal_write(0, &to_screen, 1);  // send the ascii and let terminal driver decide what to do
                if (to_screen == '\t') {
                    char_counter[curr_foreground_terminal] += 4;
                } else {
                    char_counter[curr_foreground_terminal]++;
                }
                break;
        }
    }
}
