#include "terminal.h"
#include "terminals.h"

#include "keyboard.h"
#include "lib.h"
#include "types.h"

/*
 * terminal_read
 *   DESCRIPTION: reads from the keyboard buffer into buf, return number of bytes read
 *   INPUTS: fd - file descriptor
 *           buf - buffer to write into
 *           nbytes - bytes to be write
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written to buf
 *                 -1 for unsuccessful operation
 *   SIDE EFFECTS: none
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) {
    if (buf == 0) {  // checking nullptr
        return -1;
    } else if (nbytes == 0) {  // return immediately if nbytes = 0
        return 0;
    }
    int counter = 0;                                              // counts how many characters actually read
    int read_bytes;
    int buffer_size;
    if (nbytes <= 127){
        read_bytes = nbytes;
        buffer_size = read_bytes + 1;
    } else {
        read_bytes = 128;
        buffer_size = 128;
    }
    char temp[buffer_size];                                       // temp buffer for storing the characters we have read
    int i;                                                        // loop index

    // initialize the temp buffer to be all 0s
    for (i = 0; i < buffer_size; i++) {
        temp[i] = 0;
    }

    // block until enter has been pressed
    while (!enter_flag[curr_foreground_terminal]) {
    };

    for (i = 0; i < length[curr_foreground_terminal]; i++) {  // traverse through the current line
        if (i < read_bytes) {
            // fill in the temp buffer as much as it can
            int index = (keyboard_buffer_head[curr_foreground_terminal] + i) % KEYBOARD_BUFFER_SIZE;  // calculates the correct index
            temp[counter] = keyboard_buffer[curr_foreground_terminal][index];                         // populate temp buffer with keyboard buffer content
            counter += 1;                                                   // increment counter
        }
    }
    // the buffer has read nbytes characters but has not seen a newline
    // excluding the case of nbytes == 128
    if (counter == read_bytes && temp[counter - 1] != '\n' && counter != KEYBOARD_BUFFER_SIZE) {
        temp[counter] = '\n';
    }

    // we have consumed the current line, moving on to the next and reset the variables
    keyboard_buffer_head[curr_foreground_terminal] = keyboard_buffer_tail[curr_foreground_terminal];
    enter_flag[curr_foreground_terminal] = 0;
    length[curr_foreground_terminal] = 0;

    // copy buffer_size number of characters into input buf
    memcpy(buf, temp, buffer_size);
    return counter;
}

/*
 * terminal_write
 *   DESCRIPTION: writes to the screen from buf, return number of bytes written or -1
 *   INPUTS: fd - file descriptor
 *           buf - buffer to read from
 *           nbytes - bytes to be read
 *   OUTPUTS: none
 *   RETURN VALUE: number of bytes written to screen
 *                 -1 for unsuccessful operation
 *   SIDE EFFECTS: none
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) {
    if (buf == 0) {  // checking nullptr
        return -1;
    }
    int i;                  // loop index
    char inputBuf[nbytes];  // create a buffer of nbytes
    for (i = 0; i < nbytes; i++) {
        // initialize the inputBuf buffer to be all 0s
        inputBuf[i] = 0;
    }
    memcpy(inputBuf, buf, nbytes);  // copy from buf to inputBuf
    int counter = 0;                // keeps track of how many character have been printed
    for (i = 0; i < nbytes; i++) {
        // loop nbytes times and print to the next position on screen
        char nextChar = inputBuf[i];
        if (nextChar != 0){
            putc(nextChar);
            counter += 1;
        }
    }
    return counter;
}

/*
 * terminal_open
 *   DESCRIPTION: initializes terminal variables
 *   INPUTS: filename - name of the device file
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful operation
 *   SIDE EFFECTS: none
 */
int32_t terminal_open(const uint8_t* filename) {
    int i;  // loop index
    for (i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
        // set every index of keyboard buffer to 0
        keyboard_buffer[curr_foreground_terminal][i] = 0;
    }
    // reset all keyboard related variables to 0
    keyboard_buffer_head[curr_foreground_terminal] = 0;
    keyboard_buffer_tail[curr_foreground_terminal] = 0;
    length[curr_foreground_terminal] = 0;
    enter_flag[curr_foreground_terminal] = 0;
    return 0;
}

/*
 * terminal_close
 *   DESCRIPTION: clears terminal variables
 *   INPUTS: fd - file descriptor
 *   OUTPUTS: none
 *   RETURN VALUE: 0 for successful operation
 *   SIDE EFFECTS: none
 */
int32_t terminal_close(int32_t fd) {
    int i;  // loop index
    for (i = 0; i < KEYBOARD_BUFFER_SIZE; i++) {
        // set every index of keyboard buffer to 0
        keyboard_buffer[curr_foreground_terminal][i] = 0;
    }
    // reset all keyboard related variables to 0
    keyboard_buffer_head[curr_foreground_terminal] = 0;
    keyboard_buffer_tail[curr_foreground_terminal] = 0;
    length[curr_foreground_terminal] = 0;
    enter_flag[curr_foreground_terminal] = 0;
    return 0;
    return 0;
}
