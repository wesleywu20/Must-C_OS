#ifndef _FILESYS_H
#define _FILESYS_H

#include "types.h"
#include "pcb.h"

#define FILENAME_LEN 32             // max number bytes for file name 
#define NUM_DATA_BLOCKS 1023        // max number of data blocks
#define NUM_DIR_ENTRIES 63          // max number of directory entries, including directory entry for directory itself
#define BB_RES_BYTES 52             // reserved bytes in the boot block
#define DIR_ENTRY_RES_BYTES 24      // reserved bytes in each directory entry
#define RTC_FILE_TYPE 0             // file type number for RTC
#define DIR_FILE_TYPE 1             // file type number for directory
#define REG_FILE_TPYE 2             // file type number for regular file
#define BLOCK_SIZE 4096             // size of each block (inode, data block, boot block) in bytes
#define BOOT_BLOCK_OFFSET BLOCK_SIZE // offset from start of array to index into inodes and data blocks (size of boot block and '.' inode)
#define FD_SIZE 8                  // size of file descriptor array (PCB)
#define STDIN_PCB_IDX 0             // index of stdin (keyboard input) file in PCB
#define STDOUT_PCB_IDX 1            // index of stdout (terminal output) file in PCB
#define ELF_MAGIC_LEN 4             // length of ".ELF" magic at beginning of executable files
#define PROG_EIP_START_BYTE 24      // offset into executables at which the prog_eip address is found

extern uint8_t ELF_MAGIC[ELF_MAGIC_LEN]; // magic expected at beginning of executable files
extern uint32_t pid_count;

/* 
 * each entry in the filesystem (62 max, excluding the dentry for the directory '.')
 *  -filename (32B, split into bytes)
 *  -filetype (4B)
 *  -inode_num (4B)
 *  -holds 24B reserved
 */
typedef struct dentry {
    int8_t filename[FILENAME_LEN];
    int32_t filetype;
    int32_t inode_num;
    int8_t reserved[DIR_ENTRY_RES_BYTES];
} dentry_t;

/* 
 * first entry in filesystem goes to boot block with statistics of the filesystem
 *  -number directories
 *  -number inodes
 *  -number data blocks
 *  -holds 55B reserved
 *  -holds 64B of directory entries, including the entry for the directory itself (flat filesystem)
 */
typedef struct boot_block {
    int32_t dir_count;
    int32_t inode_count;
    int32_t data_count;
    int8_t reserved[BB_RES_BYTES];
    dentry_t direntries[NUM_DIR_ENTRIES];
} boot_block_t;

/* 
 * each inode for each file
 *  -length (number of data blocks -> 4B)
 *  -data_block_num (0, 1, 2,... -> 4B each)
 */
typedef struct inode {
    int32_t length;
    int32_t data_block_num [NUM_DATA_BLOCKS];
} inode_t;

/*
 * struct for data block
 */
typedef struct data_block {
    uint8_t data[BLOCK_SIZE];
} data_block_t;

/*
 * filesystem struct to hold pointers to 
 * each section of filesystem
 */
typedef struct filesystem {
    boot_block_t * boot_block;
    inode_t * inodes;
    data_block_t * data_blocks;
} filesystem_t;

// /*
//  * struct for file_descriptor with file descriptor attributes
//  */
// typedef struct file_descriptor {
//     int32_t * fileops_table_ptr;
//     int32_t inode;
//     int32_t file_pos;
//     int32_t flags;
// } file_descriptor_t;

// all the file operations (file and directory)
typedef int32_t (*open_t)(const uint8_t *);
typedef int32_t (*read_t)(int32_t, void *, int32_t);
typedef int32_t (*write_t)(int32_t, const void *, int32_t);
typedef int32_t (*close_t)(int32_t);

/*
 * struct for file operations for file descriptors
 */
typedef struct fileops_table {
    open_t fd_open;
    read_t fd_read;
    write_t fd_write;
    close_t fd_close;
} fileops_table_t;

/* CHECK FILESYSTEM.C FOR FUNCTION INTERFACES */
extern filesystem_t filesystem;
int32_t get_filesys(uint32_t fs_addr);
int32_t read_dentry_by_name(const uint8_t * fname, dentry_t * dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t * dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);
int32_t write_data(uint32_t inode, uint32_t offset, uint8_t * buf, uint32_t length);

extern file_descriptor_t TEST_PCB[FD_SIZE];
int32_t start_process(file_descriptor_t * PCB);

int32_t file_open(const uint8_t * filename);
int32_t file_read(int32_t fd, void * buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_close(int32_t fd);

int32_t dir_open(const uint8_t * dirname);
int32_t dir_read(int32_t fd, void * buf, int32_t nbytes);
int32_t dir_write(int32_t fd, const void * buf, int32_t nbytes);
int32_t dir_close(int32_t fd);

int32_t exec_file_check(const uint8_t * command, uint32_t * prog_eip, dentry_t * dir_entry);

#endif
