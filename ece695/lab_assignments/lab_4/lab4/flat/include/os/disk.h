#ifndef __DISK_H__
#define __DISK_H__

// Student: Change the filename to ensure it does not get rewritten by
// someone else.

// Name of file which represents the "hard disk".
#define DISK_FILENAME "/tmp/ece695jain490.img"

// Number of bytes in one physical disk block
#define DISK_BLOCKSIZE 512 

typedef struct disk_block {
  char data[DISK_BLOCKSIZE]; // This assumes that DISK_BLOCKSIZE is a multiple of 4 (for byte alignment)
} disk_block;


// Total size of this disk, in units of 512-byte blocks
#define DISK_MAXSIZE 0x4000000  // 64MB
#define DISK_NUMBLOCKS (DISK_MAXSIZE / DISK_BLOCKSIZE) // STUDENT: Calculate the correct no of blocks and write. This value is wrong

#define DISK_SUCCESS 1
#define DISK_FAIL -1

int DiskBytesPerBlock();
int DiskSize();
int DiskCreate();
int DiskWriteBlock (uint32 blocknum, disk_block *b);
int DiskReadBlock (uint32 blocknum, disk_block *b);

#endif
