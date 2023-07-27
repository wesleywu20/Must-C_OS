#include "filesystem.h"

#include "lib.h"
#include "rtc.h"
#include "terminal.h"

// filesystem struct to hold pointers to fs sections (boot block, inodes, data blocks)
filesystem_t filesystem;

// default operations
int32_t default_open(const uint8_t *dirname);
int32_t default_read(int32_t fd, void *buf, int32_t nbytes);
int32_t default_write(int32_t fd, const void *buf, int32_t nbytes);
int32_t default_close(int32_t fd);

// fileops_table_t for rtc, stdin, and stdout
const fileops_table_t stdinOps_table = {
    .fd_open = &default_open,
    .fd_read = &terminal_read,
    .fd_write = &default_write,
    .fd_close = &default_close,
};
const fileops_table_t stdoutOps_table = {
    .fd_open = &default_open,
    .fd_read = &default_read,
    .fd_write = &terminal_write,
    .fd_close = &default_close,
};
const fileops_table_t fileops = {
    .fd_open = &file_open,
    .fd_read = &file_read,
    .fd_write = &file_write,
    .fd_close = &file_close,
};
const fileops_table_t dirops = {
    .fd_open = &dir_open,
    .fd_read = &dir_read,
    .fd_write = &dir_write,
    .fd_close = &dir_close,
};

/*
 * get_filesys
 *  DESCRIPTION: points pointers to the beginning of boot
 *      block, inodes, and data blocks to organize the fs
 *      in a struct and make indexing easier
 *  INPUTS:
 *      fs_addr -- address of fs in memory
 *  OUTPUTS:
 *      filesystem -- struct that holds pointers to each
 *          of the three sections
 *  RETURN VALUE: 0 for success, -1 for failure
 *  SIDE EFFECTS: fills in filesystem struct
 */
int32_t get_filesys(uint32_t fs_addr) {
    // make sure pointer is valid
    if ((void *)fs_addr == NULL) {
        return -1;
    }
    // fill in filesystem struct using pointer passed in
    filesystem.boot_block = (boot_block_t *)((uint8_t *)fs_addr);
    filesystem.inodes = (inode_t *)((uint8_t *)filesystem.boot_block + BLOCK_SIZE);
    filesystem.data_blocks = (data_block_t *)((uint8_t *)filesystem.boot_block + BLOCK_SIZE * (filesystem.boot_block->inode_count + 1));
    return 0;
}

/*
 * read_dentry_by_name
 *  DESCRIPTION: uses name of file to find the dentry in boot
 *      block and fill in a dentry struct with the file's dentry
 *  INPUTS:
 *      fname -- name of file
 *      dentry -- dentry struct to fill in
 *  OUTPUTS:
 *      dentry -- filled in dentry struct
 *  RETURN VALUE: 0 for success, -1 for failure
 *  SIDE EFFECTS: locates and extracts dentry information
 */
int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
    // make sure file name, dentry passed in are valid
    // make sure filesystem struct valid
    if (fname == NULL || dentry == NULL || filesystem.boot_block == NULL) {
        printf("Invalid parameter to read_dentry_by_name.\n");
        return -1;
    }
    int i, status;
    dentry_t dir_entry;
    // go through dentries in boot block and match file names to locate file
    for (i = 0; i < filesystem.boot_block->dir_count; i++) {
        // if read_dentry_by_index call a success fill in passed in dentry
        status = read_dentry_by_index(i, &dir_entry);
        if (status == 0 && !strncmp(dir_entry.filename, (int8_t *)fname, FILENAME_LEN)) {
            *dentry = dir_entry;
            // return success
            return 0;
        }
    }
    // if dentry not found, return failure
    return -1;
}

/*
 * read_dentry_by_index
 *  DESCRIPTION: uses index number of file to find the dentry
 *      in boot (NOT inode number)
 *  INPUTS:
 *      index -- index of file in dentry
 *      dentry -- dentry struct to fill in
 *  OUTPUTS:
 *      dentry -- filled in dentry struct
 *  RETURN VALUE: 0 for success, -1 for failure
 *  SIDE EFFECTS: locates and extracts dentry information
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {
    // make sure index and dentry and fs struct are valid
    if (index < 0 || index >= filesystem.boot_block->dir_count || dentry == NULL || filesystem.boot_block == NULL) {
        // return failure if not
        return -1;
    }
    // index into filesystem using passed in index to fill in dentry struct
    *dentry = filesystem.boot_block->direntries[index];
    // return success
    return 0;
}

/*
 * read_data
 *  DESCRIPTION: reads data from the specified file (using inode)
 *      using a given offset and read length (in bytes). read data
 *      stored in buffer and number of bytes read returned.
 *  INPUTS:
 *      inode -- number inode of file we want to read from
 *      offset -- offset from beginning of file in bytes
 *      buf -- store read data in
 *      length -- how many bytes we want to read from file
 *  OUTPUTS:
 *      buf -- buffer that holds read data
 *  RETURN VALUE: number bytes read
 *  SIDE EFFECTS: fills in buffer with data that's read from file
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length) {
    int i, d;                              // for traversing
    int32_t file_length;                   // store length of file (first 4B in inode)
    uint32_t extra;                        // to store extra number bytes if length+offset is greater than file_length
    int data_block_start, data_block_end;  // start and end data blocks we're pulling from
    int32_t data_start, data_end;          // holds bytes of data to be read in the start and end data block
    inode_t *temp_inode;                   // temporary inode to hold inode info of file we want to read from
    int32_t temp_data_idx;                 // temp array to hold data_block_num attribute
    uint8_t *temp_data;                    // temp data block
    int32_t bytes_read = 0;                // number of bytes read -> for return value

    // check to make sure inode sent in is within bounds
    if (inode < 0 || inode >= NUM_DIR_ENTRIES - 1) {
        return -1;
    }

    // temp inode to hold the information of the file we want to copy data from
    temp_inode = &(filesystem.inodes[inode]);
    file_length = temp_inode->length;

    // if offset is greater than file length then EOF reached, return 0
    if (offset > file_length) {
        return 0;
    }

    // /4096 since each data block holds 4096B of file
    data_block_start = offset / BLOCK_SIZE;
    // subtract from BLOCK_SIZE since we want what's after offset
    data_start = offset % BLOCK_SIZE;

    // if length + offset is greater than file_length, need to adjust number of bytes we read
    if ((length + offset) > file_length) {
        extra = (length + offset) - file_length;
        // int division gives block number
        data_block_end = file_length / BLOCK_SIZE;
        // % by block size gives where in block
        data_end = file_length % BLOCK_SIZE;
    } else {
        data_block_end = (length + offset) / BLOCK_SIZE;
        // do not subtract from BLOCK_SIZE since we want what's leftover
        // at the beginning of the data block
        data_end = (length + offset) % BLOCK_SIZE;
    }

    // for loop from first data block to last data block
    for (d = data_block_start; d <= data_block_end; d++) {
        // get data block num
        temp_data_idx = temp_inode->data_block_num[d];
        // make sure it's in bounds or return -1
        if (temp_data_idx < 0 || temp_data_idx >= filesystem.boot_block->data_count)
            return -1;

        // store data block in a temporary var to access
        temp_data = (uint8_t *)&(filesystem.data_blocks[temp_data_idx]);

        // for only first data block -- specfic number of bytes
        if (d == data_block_start) {
            // make sure we are in bounds of the whole file length and of the data block
            for (i = 0; bytes_read < length && i + data_start < BLOCK_SIZE; i++) {
                // if data_block_start and data_block_end are the same
                if (d == data_block_end && i + data_start >= data_end) {
                    break;
                }
                buf[i] = temp_data[i + data_start];
                bytes_read++;
            }
            // go to next data block
            continue;
        }
        // any data block between start and end -- read all bytes in block
        if ((d > data_block_start) && (d < data_block_end)) {
            // make sure we are in bounds of the whole file length and of the data block
            for (i = 0; bytes_read < length && i < BLOCK_SIZE; i++) {
                // since bytes_read increments each iteration, use it to index buf
                buf[bytes_read] = temp_data[i];
                bytes_read++;
            }
            // go to next data block
            continue;
        }
        // last data block -- specific number of bytes
        if (d == data_block_end) {
            // make sure we are in bounds of the whole file length and of the data block
            for (i = 0; bytes_read < length && i < data_end; i++) {
                // since bytes_read increments each iteration, use it to index buf
                buf[bytes_read] = temp_data[i];
                bytes_read++;
            }
            // go to next data block
            continue;
        }
    }
    // return number of bytes read
    return bytes_read;
}

/*
 * write_data
 *  DESCRIPTION: does nothing since fs is read-only.
 *  INPUTS:
 *      inode -- inode number of file
 *      offset -- offset of write
 *      buf -- to put written bytes into
 *      length -- number bytes written
 *  OUTPUTS: NONE
 *  RETURN VALUE: -1 for failure
 *  SIDE EFFECTS: nothing
 */
int32_t write_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length) {
    return -1;
}

int32_t start_process(file_descriptor_t *PCB) {
    if (PCB == NULL) {
        return -1;
    }
    int i;
    // add pointer to keyboard functions (in the 0th fd) here
    PCB[STDIN_PCB_IDX].fileops_table_ptr = (int32_t *)&stdinOps_table;
    PCB[STDIN_PCB_IDX].inode = -1;
    PCB[STDIN_PCB_IDX].file_pos = -1;
    PCB[STDIN_PCB_IDX].flags = 1;
    // add pointer to terminal functions (in the 1st fd) here
    PCB[STDOUT_PCB_IDX].fileops_table_ptr = (int32_t *)&stdoutOps_table;
    PCB[STDOUT_PCB_IDX].inode = -1;
    PCB[STDOUT_PCB_IDX].file_pos = -1;
    PCB[STDOUT_PCB_IDX].flags = 1;
    // mark all other files as not in use
    for (i = 2; i < FD_SIZE; i++) {
        PCB[i].flags = 0;
    }
    return 0;
}

/*
 * file_open
 *  DESCRIPTION: opens file and places makes open,
 *      read, write, and close operations for it. uses
 *      read_dentry_by_name to locate dentry in boot block
 *      and fill in the fd for file which is placed in
 *      PCB.
 *  INPUTS:
 *      filename -- name of file
 *  OUTPUTS: NONE
 *  RETURN VALUE: fd for success, -1 for failure
 *  SIDE EFFECTS: opens directory and creates fd in PCB for
 *      directory
 */
int32_t file_open(const uint8_t *filename) {
    // make sure filename is valid
    if (filename == NULL || strlen((int8_t *)filename) > FILENAME_LEN) {
        return -1;
    }
    int fd;
    dentry_t dir_entry;
    // check for empty space in PCB
    // start from 2 since stdin and stdout always take fda[0] and fda[1]
    for (fd = 2; fd < FD_SIZE; fd++) {
        // if full (use flags to check) go to next spot
        if (curr_pcb->file_desc[fd].flags)
            continue;
        else {
            // if empty fill in fd attributes with file attributes after finding dentry
            if (!read_dentry_by_name(filename, &dir_entry)) {
                curr_pcb->file_desc[fd].inode = dir_entry.inode_num;
                curr_pcb->file_desc[fd].fileops_table_ptr = (int32_t *)(&fileops);
                curr_pcb->file_desc[fd].file_pos = 0;
                curr_pcb->file_desc[fd].flags = 1;
                // return fd just made
                return fd;
            }
            break;
        }
    }
    // no empty spots, more files cannot be opened, return -1 for failure
    return -1;
}

/*
 * file_read
 *  DESCRIPTION: reads a file entry using
 *      read_dentry_by_index. uses inode
 *      position in fd to decide what file to read
 *      and file pos to tell offset to read from (updated per-read).
 *  INPUTS:
 *      fd -- file descriptor for file
 *      buf - buffer to put read info in
 *      nbytes -- length of buffer
 *  OUTPUTS:
 *      buf -- with read info
 *  RETURN VALUE: 0 for success, -1 for failure
 *  SIDE EFFECTS: reads next directory entry and extracts
 *      info into buffer
 */

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {
    // make sure all paramaters are valid, bounds check
    if ((fd < 0 || fd >= FD_SIZE) || buf == NULL || !curr_pcb->file_desc[fd].flags) {
        return -1;
    }
    // read data from open file and place in buffer
    int num_bytes_read = read_data(curr_pcb->file_desc[fd].inode, curr_pcb->file_desc[fd].file_pos, (uint8_t *)buf, nbytes);
    // if we read more than 0 bytes update file position
    if (num_bytes_read > 0) {
        curr_pcb->file_desc[fd].file_pos += num_bytes_read;
    }
    // will only reach here on success
    return num_bytes_read;
}

/*
 * file_write
 *  DESCRIPTION: does nothing since fs is read-only.
 *  INPUTS:
 *      fd -- file descriptor for directory
 *      buf -- to put written bytes into
 *      nbytes -- number bytes written
 *  OUTPUTS: NONE
 *  RETURN VALUE: -1 for failure
 *  SIDE EFFECTS: nothing
 */
int32_t file_write(int32_t fd, const void *buf, int32_t nbytes) {
    return -1;
}

/*
 * file_close
 *  DESCRIPTION: closes file in PCB by setting fd
 *      attributes to 0s of NULL.
 *  INPUTS:
 *      fd -- file descriptor for file
 *  OUTPUTS: NONE
 *  RETURN VALUE: 0 for success, -1 for failure
 *  SIDE EFFECTS: closes fd entry in PCB (ready for use)
 */
int32_t file_close(int32_t fd) {
    // bounds check and make sure fd passed in is valid (check flags)
    if ((fd >= 2 && fd < FD_SIZE) && curr_pcb->file_desc[fd].flags) {
        // set all the fd attributes in PCB for that fd to NULL or 0
        curr_pcb->file_desc[fd].fileops_table_ptr = NULL;
        curr_pcb->file_desc[fd].inode = 0;
        curr_pcb->file_desc[fd].file_pos = 0;
        curr_pcb->file_desc[fd].flags = 0;
    } else {
        // return -1 on failure
        return -1;
    }
    // only reach here if success
    return 0;
}

/*
 * dir_open
 *  DESCRIPTION: opens directory and places makes open,
 *      read, write, and close operations for it. uses
 *      read_dentry_by_name to locate dentry in boot block
 *      and fill in the fd for directory which is placed in
 *      PCB.
 *  INPUTS:
 *      dirname -- name of directory
 *  OUTPUTS: NONE
 *  RETURN VALUE: fd for success, -1 for failure
 *  SIDE EFFECTS: opens directory and creates fd in PCB for
 *      directory
 */
int32_t dir_open(const uint8_t *dirname) {
    // make sure dirname is valid
    if (dirname == NULL) {
        return -1;
    }
    int fd;
    dentry_t dir_entry;
    // go through PCB and find empty spot to place directory fd
    // start from 2 since stdin and stdout always take fda[0] and fda[1]
    for (fd = 2; fd < FD_SIZE; fd++) {
        // if flags 1, spot is in use, keep looking
        if (curr_pcb->file_desc[fd].flags)
            continue;
        else {
            // if not in use, grab dentry info and fill in fd appropriately
            if (!read_dentry_by_name(dirname, &dir_entry)) {
                curr_pcb->file_desc[fd].inode = dir_entry.inode_num;
                curr_pcb->file_desc[fd].fileops_table_ptr = (int32_t *)(&dirops);
                curr_pcb->file_desc[fd].file_pos = 0;
                curr_pcb->file_desc[fd].flags = 1;
                // return new fd
                return fd;
            }
            break;
        }
    }
    // only reach here on failure (can't place directory fd in PCB)
    return -1;
}

/*
 * dir_read
 *  DESCRIPTION: reads a directory entry one at a time using
 *      read_dentry_by_index. only goes in order. uses file
 *      position in fd to decide what file to read next.
 *  INPUTS:
 *      fd -- file descriptor for directory
 *      buf - buffer to put dentry information in
 *      nbytes -- length of buffer
 *  OUTPUTS:
 *      buf -- with dentry info
 *  RETURN VALUE: number of bytes read for success, 0 for
 *      failure
 *  SIDE EFFECTS: reads next directory entry and extracts
 *      info into buffer
 */
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {
    // check if given fd is in bounds and buffer is valid -- check fail conditions
    if ((fd < 0 || fd >= FD_SIZE) || buf == NULL || !curr_pcb->file_desc[fd].flags) {
        return -1;
    }
    // make sure we read 32 bytes
    int bytes_to_read = nbytes <= 32 ? nbytes : 32;
    dentry_t dir_entry;  // hold dentry info
    // if read successful
    if (!read_dentry_by_index(curr_pcb->file_desc[fd].file_pos, &dir_entry)) {
        // copy filename to buffer
        memcpy(buf, (void *)dir_entry.filename, bytes_to_read);
        // update inode number in fd
        curr_pcb->file_desc[fd].inode = dir_entry.inode_num;
        // increment file position
        curr_pcb->file_desc[fd].file_pos++;
        // return number of bytes read
        return bytes_to_read;
    } else {
        return 0;
    }
}

/*
 * dir_close
 *  DESCRIPTION: closes directory fd in PCB by setting fd
 *      attributes to 0s of NULL.
 *  INPUTS:
 *      fd -- file descriptor for directory
 *  OUTPUTS: NONE
 *  RETURN VALUE: 0 for success, -1 for failure
 *  SIDE EFFECTS: closes fd entry in PCB (ready for use)
 */
int32_t dir_close(int32_t fd) {
    // bounds check and make sure spot in PCB is in use
    if ((fd >= 2 && fd < FD_SIZE) && curr_pcb->file_desc[fd].flags) {
        // set all the fd attributes to NULL or 0
        curr_pcb->file_desc[fd].fileops_table_ptr = NULL;
        curr_pcb->file_desc[fd].inode = 0;
        curr_pcb->file_desc[fd].file_pos = 0;
        curr_pcb->file_desc[fd].flags = 0;
    } else {
        // if cannot be done, return -1 for failure
        return -1;
    }
    // only reach here if success, return 0
    return 0;
}

/*
 * dir_write
 *  DESCRIPTION: does nothing since fs is read-only.
 *  INPUTS:
 *      fd -- file descriptor for directory
 *      buf -- to put written bytes into
 *      nbytes -- number bytes written
 *  OUTPUTS: NONE
 *  RETURN VALUE: -1 for failure
 *  SIDE EFFECTS: nothing
 */
int32_t dir_write(int32_t fd, const void *buf, int32_t nbytes) {
    return -1;
}

/*
 * default_open
 *   DESCRIPTION: open function that return -1
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t default_open(const uint8_t *filename) {
    return -1;
}

/*
 * default_read
 *   DESCRIPTION: read function that return -1
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t default_read(int32_t fd, void *buf, int32_t nbytes) {
    return -1;
}

/*
 * default_write
 *   DESCRIPTION: write function that return -1
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t default_write(int32_t fd, const void *buf, int32_t nbytes) {
    return -1;
}

/*
 * default_close
 *   DESCRIPTION: close function that return -1
 *   INPUTS: ignored
 *   OUTPUTS: none
 *   RETURN VALUE: -1
 *   SIDE EFFECTS: none
 */
int32_t default_close(int32_t fd) {
    return -1;
}

uint8_t ELF_MAGIC[ELF_MAGIC_LEN] = {0x7F, 0x45, 0x4C, 0x46};
int32_t exec_file_check(const uint8_t * command, uint32_t * prog_eip, dentry_t * dir_entry) {
    uint8_t filename[FILENAME_LEN];
    uint8_t elf_magic[ELF_MAGIC_LEN];
    int i, status;
    /* check for garbage input values */
    if (command == NULL || prog_eip == NULL || dir_entry == NULL) {
        return -1;
    }
    /* copy the file name into a buffer */
    for (i = 0; i < FILENAME_LEN && (command[i] != ' ' && command[i] != '\0'); i++) {
        filename[i] = command[i];
    }
    filename[i] = '\0';
    /* get the directory entry of the file */
    status = read_dentry_by_name(filename, dir_entry);
    if (status == -1) {
        return -1;
    }
    /* read the beginning ELF_MAGIC_LEN bytes to verify 0x7F 0x45 0x4C 0x46 is seen at the beginning of the executable */
    status = read_data(dir_entry->inode_num, 0, elf_magic, ELF_MAGIC_LEN);
    if (status == -1 || strncmp((int8_t *)elf_magic, (int8_t *)ELF_MAGIC, ELF_MAGIC_LEN)) {
        return -1;
    }
    /* get prog_eip from bytes 24-27 */
    status = read_data(dir_entry->inode_num, PROG_EIP_START_BYTE, (uint8_t *)(prog_eip), 4);
    if (status == -1) {
        return -1;
    }
    /* since ELF magic seen, the file is executable and passes */
    return 0;
}
