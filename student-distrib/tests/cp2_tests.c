#include "tests.h"

#ifdef CP2

#include "../syscall_asm.h"
#include "../lib.h"
#include "../x86_desc.h"
#include "../i8259.h"
#include "../rtc.h"
#include "../terminal.h"
#include "../filesystem.h" 

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER \
    printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result) \
    printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure() {
    /* Use exception #15 for assertions, otherwise
       reserved by Intel */
    asm volatile("int $15");
}

/* Checkpoint 2 tests */

/*
 * terminal_read_test
 *   DESCRIPTION: assert that terminal_read will wait until enter is pressed and return the correct number of bytes read
 *   INPUTS: none
 *   OUTPUTS: system will wait for user to type until an enter is pressed
 *            if the output of terminal_read is correct, the test will pass
 *   SIDE EFFECTS: none
 *   COVERAGE: terminal.c keyboard.c
 */
int terminal_read_test(int num_char_to_type){
    TEST_HEADER;
    int result;
    char buf[180] = {0}; // create a buffer with all 0s
    printf("nbytes = 180, Type %d characters and hit Enter: ", num_char_to_type);
    int res = terminal_read(0, buf, 180);
    printf("return value: %d\n", res);
    printf("buf: %s\n", buf);
    if (res != num_char_to_type + 1) // the result should be (number of characters typed) + 1 because (number of characters typed) is less than nbytes
        return FAIL;

    char buf2[180] = {0};// create a buffer with all 0s
    printf("nbytes = %d, Type %d characters and hit Enter: ", num_char_to_type - 1, num_char_to_type);
    res = terminal_read(0, buf2, num_char_to_type - 1);
    printf("return value: %d\n", res);
    printf("buf: %s\n", buf2);
    if (res != num_char_to_type - 1) // the result should be (number of character typed) - 1 because the user has typed more than the buffer size
        return FAIL;

    char buf3[180] = {0};  // create a buffer with all 0s
    printf("nbytes = 128, Type more than 128 characters and hit Enter: ");
    res = terminal_read(0, buf3, 128);
    printf("return value: %d\n", res);
    printf("buf: %s\n", buf3);

    result = res == 128 ? PASS : FAIL; // the result should be 128 because the user has typed more than the size of keyboard buffer
    return result;

}

/*
 * terminal_write_test
 *   DESCRIPTION: assert that terminal_write would write the buf content to the screen and return number of characters written
 *   INPUTS: buf - buffer containing contents to write to screen
 *           nbytes - number of characters to write to the screen
 *           expected_ret_val - expected return value for terminal_write
 *   OUTPUTS: buffer content written to screen, and number of characters written is returned
 *   SIDE EFFECTS: none
 *   COVERAGE: terminal.c
 */
int terminal_write_test(char* buf, int nbytes, int expected_ret_val){
    TEST_HEADER;
    int ret_val = terminal_write(0, buf, nbytes);
    int result = ret_val == expected_ret_val ? PASS : FAIL; // the return value should be the same as the number of bytes written
    return result;
}

/*
 * rtc_test
 *   DESCRIPTION: assert that rtc_open would change the frequency to 2Hz correctly
 *   INPUTS: none
 *   OUTPUTS: frequency of rtc should change to 2Hz
 *   SIDE EFFECTS: none
 *   COVERAGE: rtc.c
 */
int rtc_test(){
    TEST_HEADER;
    int i; // loop index
    int fd = rtc_open((uint8_t*)"RTC");  // opening the device
    int time = 3;                        // changing frequency every 4 seconds
    int frequency = 2;                   // initial frequency
    while (frequency <= 1024) {
        // for each iteration, double the frequency
        rtc_write(fd, &frequency, sizeof(frequency));
        clear();
        for (i = 0; i < frequency * time; i++) {
            // stay in this frequency for "time" seconds and keep printing 1 for each interrupt
            rtc_read(fd, NULL, 0);
            putc('1');
        }
        frequency *= 2;
    }
    putc('\n'); // start a new line
    int result = PASS; // print the test passes message
    return result;
}

/*
 * fs_num_bytes_read_test
 *   DESCRIPTION: check that the number of bytes read returned
 *     equals the length (no bounds issues)
 *   INPUTS: none
 *   OUTPUTS: returns number of bytes read
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_data
 */
int fs_num_bytes_read_test() {
    TEST_HEADER;
    int32_t num_bytes_read;
    uint32_t inode = 55; // fish file
    uint32_t offset = 50; // 50 chars
    uint8_t buf[50];
    uint32_t length = 50; // 50 chars
    printf("Number bytes that should be read: %d\n", length);
    // store number bytes read
    num_bytes_read = read_data(inode, offset, buf, length); 
    printf("Number bytes read: %d\n", num_bytes_read);
    // if length matches number bytes read then success or else failure
    if(num_bytes_read == length)
        return PASS;
    else
        return FAIL;
}

/*
 * fs_offset_too_large_test
 *   DESCRIPTION: check that when the offset is larger than 
 *     the file length nothing gets read into buffercheck 
 *     that the number of bytes read returned
 *   INPUTS: none
 *   OUTPUTS: returns number of bytes read
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_data
 */
int fs_offset_too_large_test() {
    TEST_HEADER;
    int32_t num_bytes_read;
    uint32_t inode = 47; // frame1 file
    uint32_t offset = 31647964; // 31647964 chars
    uint8_t buf[50];
    uint32_t length = 50; // 50 chars
    printf("Number bytes that should be read: 0\n");
    // call read_data
    num_bytes_read = read_data(inode, offset, buf, length);
    printf("Number bytes read: %d\n", num_bytes_read);
    // since offset > file_length, no bytes shouls be read
    // pass if that's the case, fail otherwise
    if(num_bytes_read == 0)
        return PASS;
    else
        return FAIL;
}

/*
 * fs_length_overflows_test
 *   DESCRIPTION: check that when the length+offset is greater 
 *         than file_length only file_length - (length + offset) 
 *         gets printed
 *   INPUTS: none
 *   OUTPUTS: returns number of bytes read
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_data
 */
int fs_length_overflows_test() {
	TEST_HEADER;
    inode_t * temp_inode;
    uint32_t inode = 47; // frame1 file
    // get file length
    temp_inode = (inode_t *)((uint8_t *)filesystem.inodes + inode * sizeof(inode_t));
    int32_t file_length = temp_inode->length;
    uint8_t buf[50];
    uint32_t offset = file_length - 40;
    uint32_t length = 50;
    printf("Number bytes that should be read: 40\n");
    // call read_data
    int32_t num_bytes_read = read_data(inode, offset, buf, length);
    printf("Number bytes read: %d\n", num_bytes_read);
    // number of bytes read should only be what was left in file, 
    // even though length is longer
    if(num_bytes_read == 40)
        return PASS;
    else
        return FAIL;
}

/*
 * fs_read_small_file_test
 *   DESCRIPTION: reads a small file and prints the whole file
 *   INPUTS: none
 *   OUTPUTS: contents of file
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_data
 */
int fs_read_small_file_test() {
    TEST_HEADER;
    int32_t num_bytes_read;
    uint32_t inode = 47; // frame1 file
    uint32_t offset = 0; // start from beginning
    uint32_t length = 174; // 50 chars
    uint8_t buf[174];
    num_bytes_read = read_data(inode, offset, buf, length);
    int i;
    // print whole file and check manually
    for(i = 0; i < 174; i++) {
        printf("%c",buf[i]);
    }
    printf("\n");
	return PASS;
}

/*
 * fs_read_large_file_test
 *   DESCRIPTION: reads a large file and prints the whole file
 *   INPUTS: none
 *   OUTPUTS: contents of file
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_data
 */
int fs_read_large_file_test() {
    TEST_HEADER;
    int32_t num_bytes_read;
    uint32_t inode = 55; // fish file
    uint32_t offset = 0; // start from beginning
    uint32_t length = 36164; // hardcoded buffer after finding file size to make life easiser
    uint8_t buf[36164];
    num_bytes_read = read_data(inode, offset, buf, length);
    int i;
    // print whole file and check manually
    for (i = 0; i < length; i++) {
        printf("%c", buf[i]);
    }
    // while(0 != (num_bytes_read = read_data(inode, offset, buf, length))) {
    //     printf("%c", buf);
    //     offset++;
    // }
    printf("\n");
	return PASS;
}

/*
 * fs_read_executable_test
 *   DESCRIPTION: reads an executable and prints the whole file
 *   INPUTS: none
 *   OUTPUTS: contents of file
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_data
 */
int fs_read_executable_test() {
    TEST_HEADER;
    int32_t num_bytes_read;
    uint32_t inode = 50; // grep file
    uint32_t offset = 0; // start from beginning
    uint32_t length = 7000; // 50 chars
    uint8_t buf[7000];

    num_bytes_read = read_data(inode, offset, buf, length);
    int i;
    // print whole file and check output is right manually
    for (i = 0; i < length; i++) {
        printf("%c", buf[i]);
    }   
    // num_bytes_read = read_data(inode, offset, buf, length);
    // int i;
    // for (i = 0; i < 7; i++) {
    //     printf("%c", buf[i]);
    // }
    // printf("\n");
    // while(offset != 4095) {
    //     num_bytes_read = read_data(inode, offset, buf, length);
    //     if(buf[0] != NULL)
    //         printf("%c", buf[0]);
    //     offset++;
    // }
    // while(0 != (num_bytes_read = read_data(inode, offset, buf, length))) {
    //     if(buf[0] != NULL)
    //         printf("%c", buf[0]);
    //     offset++;
    // }
    printf("\noffset: %d\n", offset);
	return PASS;
}

/*
 * read_dentry_by_index_test
 *   DESCRIPTION: reads the entries in the directory by index,
 *      starting from the first file and stopping once
 *      the end of the directory has been reached
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_dentry_by_index
 */
int read_dentry_by_index_test() {
	TEST_HEADER;
	int result = PASS;
	dentry_t dir_entry;
	int i, status;
	printf("dir_count: %d\n", filesystem.boot_block->dir_count);
	// print each directory entry's file name for all the entries in the boot boot block
	for (i = 0; i < filesystem.boot_block->dir_count; i++) {
		status = read_dentry_by_index(i, &dir_entry);
		// if any read fails, return -1
        if (status == -1) {
			result = FAIL;
			return result;
		}
		printf("File name: %s\n", dir_entry.filename);
	}
	return result;
}

/*
 * read_dentry_by_index_bad_param_test
 *   DESCRIPTION: tries to read a directory entry 
 *      from an index far larger than the directory size
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_dentry_by_index
 */
int read_dentry_by_index_bad_param_test() {
	TEST_HEADER;
	int result = PASS;
	dentry_t dir_entry;
	int status;
    // try to read from index 391 in the boot block --
    // much greater than directory size
	status = read_dentry_by_index(391, &dir_entry);
	// should return -1, if not, fail the test
    if (status != -1) 
		result = FAIL;
	return result;
}

/*
 * read_dentry_by_name_test
 *   DESCRIPTION: tries to read two directory entries by name
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_dentry_by_name
 */
int read_dentry_by_name_test() {
	TEST_HEADER;
	int result = PASS;
	dentry_t dir_entry;
	int status;
    // try to read grep file
	status = read_dentry_by_name((uint8_t *)"grep", &dir_entry);
	if (status == -1) {
		result = FAIL;
		return result;
	}
    // print file name and inode num to confirm grep was retrieved by read
	printf("File name: %s\n", dir_entry.filename);
	printf("Inode num: %d\n", dir_entry.inode_num);
	status = read_dentry_by_name((uint8_t *)"verylargetextwithverylongname.tx", &dir_entry);
	if (status == -1) {
		result = FAIL;
		return result;
	}
    // print file name -- copy to null terminated buffer since
    // verylargetext... name is not null terminated and has slight
    // bug when printed (prints a smiley face at the end)
    uint8_t filename[33];
    filename[32] = '\0';
    memcpy(filename, dir_entry.filename, FILENAME_LEN);
	printf("File name: %s\n", filename);
	return result;
}

/*
 * read_dentry_by_name_bad_param_test
 *   DESCRIPTION: tries to read a directory entry that has a NULL filename
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- read_dentry_by_index
 */
int read_dentry_by_name_bad_param_test() {
	TEST_HEADER;
	int result = PASS;
	dentry_t dir_entry;
	int status;
    // try to read a directory entry where the passed in name is NULL
	status = read_dentry_by_name(NULL, &dir_entry);
	// should return -1, if not, fail the test
    if (status != -1) {
		result = FAIL;
	}
	return result;
}

// declare the global test PCB for cp2
file_descriptor_t TEST_PCB[PCB_SIZE];

/*
 * file_open_test
 *   DESCRIPTION: test that a file can be opened by name
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: opens file and makes fd
 *   COVERAGE: filesystem.c -- file_open
 */
int file_open_test() {
	TEST_HEADER;
	int result = PASS;
    // use file open to open fish, and print to confirm the right file was retrieved
	int fd = file_open((uint8_t *)"fish");
	printf("fish fd: %d\n", fd);
	printf("fish inode: %d\n", TEST_PCB[fd].inode);
	printf("fish file pos: %d\n", TEST_PCB[fd].file_pos);
	printf("fish fd flags: %d\n", TEST_PCB[fd].flags); // print to make sure file marked as in-use
	if (fd == -1) { // if unable to retrieve, fail test
		result = FAIL;
	}
	return result;
}

/*
 * open_nonexistent_file_test
 *   DESCRIPTION: tries to open a file that doesn't exist
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_open
 */
int open_nonexistent_file_test() {
    TEST_HEADER;
    int result = PASS;
    // try to open non-existent file (abc is not in fsdir)
    int fd = file_open((uint8_t *)"abc");
    if (fd != -1) { // should return -1, if not, fail test
        result = FAIL;
    }
    return result;
}

/*
 * file_open_invalid_fname_test
 *   DESCRIPTION: tries to pass in NULL as directory name
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_open
 */
int file_open_invalid_fname_test() {
    TEST_HEADER;
    int result = PASS;
    // try to open a file where name passed in is NULL
    int fd = file_open(NULL);
    if (fd != -1) { // should return -1, if not, fail test
        result = FAIL;
    }
    return result;
}

/*
 * file_close_test
 *   DESCRIPTION: tries to close file after opening it
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_close
 */
int file_close_test() {
	TEST_HEADER;
	int result = PASS;
	int fd = file_open((uint8_t *)"frame0.txt");
	if (fd == -1) {
		result = FAIL;
		return result;
	}
	int status = file_close(fd);
	printf("frame0.txt fd flag: %d\n", TEST_PCB[fd].flags);
	if (status == -1 || TEST_PCB[fd].flags != 0) {
		result = FAIL;
	}
	return result;
}

/*
 * close_nonopen_file_test
 *   DESCRIPTION: tries to close a file that isn't open in the PCB
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_close
 */
int close_nonopen_file_test() {
    TEST_HEADER;
    int result = PASS;
    int status = file_close(7);
    if (status != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * file_close_invalid_fd_test
 *   DESCRIPTION: tries to close file by passing in 
 *      an invalid fd number
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_close
 */
int file_close_invalid_fd_test() {
    TEST_HEADER;
    int result = PASS;
    int status = file_close(391);
    if (status != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * file_read_test
 *   DESCRIPTION: tries to read a file
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_read
 */
int file_read_test() {
    TEST_HEADER;
    int result = PASS;
    int fd = file_open((uint8_t *)"frame1.txt"); // open frame1 to setup fd
    printf("filename: frame1.txt\n");
    int i;
    int status;
    if (fd == -1) { // if unable to open frame1.txt, fail test
        return FAIL;
    }
    // read frame1.txt 29 bytes at a time and print out the buffer after reading
    // since frame1.txt file size is divisible by 29, and to confirm that 
    // the file descriptor file position is being updated correctly
    uint8_t buf[29];
    while(!(status = file_read(fd, (void *)buf, 29))) {
        for (i = 0; i < 29; i++) {
            printf("%c", buf[i]);
        }
    }
    return result;
}

/*
 * read_nonopen_file_test
 *   DESCRIPTION: tries to read a file not open in the PCB
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_read
 */
int read_nonopen_file_test() {
    TEST_HEADER;
    int result = PASS;
    void * buf;
    // try to read from file descriptor 6 in PCB
    // (when group of tests is ran, no files should be opened)
    int status = file_read(6, buf, 391);
    if (status != -1) { // if it didn't return -1, fail test
        result = FAIL;
    }
    return result;
}

/*
 * file_read_invalid_param_test
 *   DESCRIPTION: tries to read a file into a NULL buffer
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_read
 */
int file_read_invalid_param_test() {
    TEST_HEADER;
    int result = PASS;
    // try to read into NULL buffer
    int status = file_read(6, NULL, 391);
    if (status != -1) { // if it didn't return -1, fail test
        result = FAIL;
    }
    return result;
}

/*
 * file_write_test
 *   DESCRIPTION: tries to write to a file
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- file_write
 */
int file_write_test() {
	TEST_HEADER;
	int result = PASS;
	int status = file_write(0, NULL, 391);
	if (status != -1) { // check that file_write does nothing and returns -1
		result = FAIL;
	}
	return result;
}

// make directory fd
static int dir_fd;

/*
 * dir_open_test
 *   DESCRIPTION: test that directory can be opened
 *   INPUTS: NONE
 *   OUTPUTS: NONE
 *   SIDE EFFECTS: opens directory and makes fd
 *   COVERAGE: filesystem.c -- dir_open
 */
int dir_open_test() {
    TEST_HEADER;
    int result = PASS;
    // call dir_open
    dir_fd = dir_open((uint8_t *)".");
    // unless fd is -1, pass
    if (dir_fd == -1) {
        result = FAIL;
    }
    return result;
}

/*
 * open_nonexistent_dir_test
 *   DESCRIPTION: tries to open a directory that doesn't exist
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_open
 */
int open_nonexistent_dir_test() {
    TEST_HEADER;
    int result = PASS;
    // try to open directory that doesn't exist
    dir_fd = dir_open((uint8_t *)"abcdefg");
    // will pass unless dir_open does not fail (return -1)
    if (dir_fd != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * dir_open_bad_param_test
 *   DESCRIPTION: tries to pass in NULL as directory name
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_open
 */
int dir_open_bad_param_test() {
    TEST_HEADER;
    int result = PASS;
    // pass in NULL
    int fd = dir_open(NULL);
    // if dir_open does not fail, fail test
    if (fd != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * dir_read_test
 *   DESCRIPTION: tries to read a directory entry
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_read
 */
int dir_read_test() {
    TEST_HEADER;
    int result = PASS;
    uint8_t filename[FILENAME_LEN];
    int i;
    // print all the directory entries in the boot block one at 
    // a time by calling dir_read each time
    for (i = 0; i < filesystem.boot_block->dir_count; i++) {
        int status = dir_read(dir_fd, (void *)filename, 391);
        // make sure dir_read does not return -1
        // if it does, test fails
        if (status == -1) {
            result = FAIL;
            return result;
        }
        // print file name
        printf("File name: %s\n", filename);
    }
    return result;
}

/*
 * dir_read_on_nondir_test
 *   DESCRIPTION: tries to call dir_read on something that's 
 *      not a file descriptor type
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_read
 */
int dir_read_on_nondir_test() {
    TEST_HEADER;
    int result = PASS;
    void * buf;
    // send in 6 for fd
    int status = dir_read(6, buf, 391);
    // if dir_read does not fail, fail test
    if (status != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * dir_read_bad_param_test
 *   DESCRIPTION: tries to read a directory entry while passing in 
 *      NULL to filename
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_read
 */
int dir_read_bad_param_test() {
    TEST_HEADER;
    int result = PASS;
    // pass in NULL for filename
    int status = dir_read(4, NULL, 391);
    // if call doesn't fail, fail test
    if (status != -1) {
        result = FAIL;
    }
    return result;
}


/*
 * dir_write_test
 *   DESCRIPTION: tries to write to directory
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_write
 */
int dir_write_test() {
    TEST_HEADER;
    int result = PASS;
    // dir_write should always fail, read-only fs
    int status = dir_write(dir_fd, NULL, 0);
    // if call doesn't fail, fail test
    if (status != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * dir_close_test
 *   DESCRIPTION: tries to close directory
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_close
 */
int dir_close_test() {
    TEST_HEADER;
    int result = PASS;
    int status = dir_close(dir_fd);
    // print attributes of fd after directory is closed
    // should all be NULL and 0
    printf("directory fd flags: %d\n", TEST_PCB[dir_fd].flags);
    // if call failed or flags is not 0, test failed and directory 
    // not closed properly
    if (status == -1 || TEST_PCB[dir_fd].flags != 0) {
        result = FAIL;
    } 
    return result;
}

/*
 * close_nonexistent_dir_test
 *   DESCRIPTION: tries to close a directory that doesn't exist
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_close
 */
int close_nonexistent_dir_test() {
    TEST_HEADER;
    int result = PASS;
    // pass into dir_close a nonexistent directory
    int status = dir_close(6);
    // if call doesn't fail, fail test
    if (status != -1) {
        result = FAIL;
    }
    return result;
}

/*
 * dir_close_bad_param_test
 *   DESCRIPTION: tries to close directory by passing in 
 *      an invalid parameter
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: filesystem.c -- dir_close
 */
int dir_close_bad_param_test() {
    TEST_HEADER;
    int result = PASS;
    // pass in bad parameter to dir_close
    int status = dir_close(9000);
    // if call doesn't fail, fail test
    if (status != -1) {
        result = FAIL;
    }
    return result;
}

/* Test suite entry point */
void launch_tests_cp2() {
    clear();

    // TEST_OUTPUT("terminal_read_test", terminal_read_test(5));
    // TEST_OUTPUT("terminal_write_test", terminal_write_test("hello world\n", 12, 12));
    // TEST_OUTPUT("terminal_write_test", terminal_write_test("hello \0world\n", 13, 12));
    // TEST_OUTPUT("rtc_test", rtc_test());
    // TEST_OUTPUT("fs_num_bytes_read_test", fs_num_bytes_read_test());
    // TEST_OUTPUT("fs_offset_too_large_test", fs_offset_too_large_test());
    // TEST_OUTPUT("fs_length_overflows_test", fs_length_overflows_test());
    // TEST_OUTPUT("fs_read_small_file_test", fs_read_small_file_test());
    // TEST_OUTPUT("fs_read_large_file_test", fs_read_large_file_test());
    // TEST_OUTPUT("fs_read_executable_test", fs_read_executable_test());
    TEST_OUTPUT("read_dentry_by_index_test", read_dentry_by_index_test());
	// TEST_OUTPUT("read_dentry_by_name_test", read_dentry_by_name_test());
	// TEST_OUTPUT("read_dentry_by_index_bad_param_test", read_dentry_by_index_bad_param_test());
	// TEST_OUTPUT("read_dentry_by_name_bad_param_test", read_dentry_by_name_bad_param_test());
	// TEST_OUTPUT("file_open_test", file_open_test());
    // TEST_OUTPUT("file_read_test", file_read_test());
	// TEST_OUTPUT("file_write_test", file_write_test());
	// TEST_OUTPUT("file_close_test", file_close_test());
    // TEST_OUTPUT("dir_open_test", dir_open_test());
    // TEST_OUTPUT("dir_read_test", dir_read_test());
    // TEST_OUTPUT("dir_write_test", dir_write_test());
    // TEST_OUTPUT("dir_close_test", dir_close_test());

    // TEST_OUTPUT("open_nonexistent_file_test", open_nonexistent_file_test());
    // TEST_OUTPUT("file_open_invalid_fname_test", file_open_invalid_fname_test());
    // TEST_OUTPUT("close_nonopen_file_test", close_nonopen_file_test());
    // TEST_OUTPUT("file_close_invalid_fd_test", file_close_invalid_fd_test());
    // TEST_OUTPUT("read_nonopen_file_test", read_nonopen_file_test());
    // TEST_OUTPUT("file_read_invalid_param_test", file_read_invalid_param_test());

    // TEST_OUTPUT("open_nonexistent_dir_test", open_nonexistent_dir_test());
    // TEST_OUTPUT("dir_open_bad_param_test", dir_open_bad_param_test());
    // TEST_OUTPUT("dir_read_on_nondir_test", dir_read_on_nondir_test());
    // TEST_OUTPUT("dir_read_bad_param_test", dir_read_bad_param_test());
    // TEST_OUTPUT("close_nonexistent_dir_test", close_nonexistent_dir_test());
    // TEST_OUTPUT("dir_close_bad_param_test", dir_close_bad_param_test());
}


#endif
