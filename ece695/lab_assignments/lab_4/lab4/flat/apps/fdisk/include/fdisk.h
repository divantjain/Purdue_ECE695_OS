#ifndef __FDISK_H__
#define __FDISK_H__

typedef unsigned int uint32;

#include "dfs_shared.h" // This gets us structures and #define's from main filesystem driver

#define FDISK_INODE_BLOCK_START 1 // Starts after super block (which is in file system block 0, physical block 1)
#define FDISK_INODE_NUM_BLOCKS 16 // STUDENT :Number of file system blocks to use for inodes
#define FDISK_NUM_INODES 128 //STUDENT: define this
#define FDISK_FBV_BLOCK_START 17 //STUDENT: define this
#define FDISK_BOOT_FILESYSTEM_BLOCKNUM 0 // Where the boot record and superblock reside in the filesystem

#ifndef NULL
#define NULL (void *)0x0
#endif

//STUDENT: define additional parameters here, if any
#define FDISK_FILESYSTEM_MAXSIZE (0x4000000)
#define FDISK_FILESYSTEM_BLOCKSIZE (1024)
#define FDISK_FILESYSTEM_NUM_BLOCKS (FDISK_FILESYSTEM_MAXSIZE / FDISK_FILESYSTEM_BLOCKSIZE)

#define FDISK_MBR_NUM_BLOCKS (1)

#define FDISK_BITS_PER_BYTE (8)
#define FDISK_FBV_BYTES_PER_ENTRY (4)
#define FDISK_FBV_BITS_PER_ENTRY (FDISK_FBV_BYTES_PER_ENTRY * FDISK_BITS_PER_BYTE)
#define FDISK_FBV_MAX_NUM_WORDS (FDISK_FILESYSTEM_NUM_BLOCKS / FDISK_FBV_BITS_PER_ENTRY)
#define FDISK_FBV_NUM_BLOCKS (FDISK_FILESYSTEM_NUM_BLOCKS / (FDISK_FILESYSTEM_BLOCKSIZE * FDISK_BITS_PER_BYTE))

#endif
