#include "usertraps.h"
#include "misc.h"

#include "fdisk.h"

dfs_superblock sb;
//dfs_inode inodes[DFS_INODE_MAX_NUM];
uint32 freeblockvector[FDISK_FBV_MAX_NUM_WORDS];

int diskblocksize = 0; // These are global in order to speed things up
int disksize = 0;      // (i.e. fewer traps to OS to get the same number)

int FdiskWriteBlock(uint32 blocknum, dfs_block *b); //You can use your own function. This function 
//calls disk_write_block() to write physical blocks to disk

void main (int argc, char *argv[])
{
  int i, j;
  int freeblockvector_offset;
  int num_used_blocks;
  int diskblocksize_returned;
  dfs_block block;

  // STUDENT: put your code here. Follow the guidelines below. They are just the main steps. 
  // You need to think of the finer details. You can use bzero() to zero out bytes in memory

  //1) Initializations and argc check
  if (argc != 1) {
    Printf("Usage: %s\n", argv[0]);
    Exit();
  }

  // 2) Need to invalidate filesystem before writing to it to make sure that the OS
  // doesn't wipe out what we do here with the old version in memory
  // You can use dfs_invalidate(); but it will be implemented in Problem 2. You can just do 
  // sb.valid = 0
  //sb.valid = 0;
  dfs_invalidate();
  
  disksize = disk_size();
  diskblocksize = disk_blocksize();

  // 3) Make sure the disk exists before doing anything else. You can use disk_create()
  disk_create();
  
  // 4) Write all inodes as not in use and empty (Write zeros to all the
  // dfs blocks corresponding to the inodes using FdiskWriteBlock)
  bzero((char *)&block, FDISK_FILESYSTEM_BLOCKSIZE);
  for (i=FDISK_INODE_BLOCK_START; i<(FDISK_INODE_BLOCK_START + FDISK_INODE_NUM_BLOCKS); i++)
  {
    diskblocksize_returned = FdiskWriteBlock(i, &block);
  }

  // 5) Next, setup free block vector (fbv) and write free block vector to the disk
  // 5.1) Mark all the blocks used by file system as "Used"
  // 5.2) Mark all other blocks as "Unused"
  // 5.3) Use FdiskWriteBlock to write to disk.
  for (i=0; i<=FDISK_FBV_MAX_NUM_WORDS; i++)
  {
    freeblockvector[i] = 0x00000000;
  }

  num_used_blocks = (FDISK_MBR_NUM_BLOCKS + FDISK_INODE_NUM_BLOCKS + FDISK_FBV_NUM_BLOCKS);
  for (i=0; i<=(num_used_blocks / FDISK_FBV_BITS_PER_ENTRY); i++)
  {
    if(i == (num_used_blocks / FDISK_FBV_BITS_PER_ENTRY))
    {
      for (j=0; j<(num_used_blocks % FDISK_FBV_BITS_PER_ENTRY); j++)
      {
        freeblockvector[i] = (freeblockvector[i] | (1 << j));
      }
    }
    else
    {
      for (j=0; j<FDISK_FBV_BITS_PER_ENTRY; j++)
      {
        freeblockvector[i] = (freeblockvector[i] | (1 << j));
      }
    }
    //Printf("freeblockvector[%d] = %d\n", i, freeblockvector[i]);
  }
/*
  for (i=0; i<=FDISK_FBV_MAX_NUM_WORDS; i++)
  {
    Printf("freeblockvector[%d] = %d\n", i, freeblockvector[i]);
  }
*/
  for (i=FDISK_FBV_BLOCK_START; i<(FDISK_FBV_BLOCK_START + FDISK_FBV_NUM_BLOCKS); i++)
  {
    bzero((char *)&block, FDISK_FILESYSTEM_BLOCKSIZE);
    freeblockvector_offset = ((i - FDISK_FBV_BLOCK_START) * (FDISK_FILESYSTEM_BLOCKSIZE / FDISK_FBV_BYTES_PER_ENTRY));
    //Printf("freeblockvector_offset = %d\n", freeblockvector_offset);
    //Printf("(char *)(freeblockvector + freeblockvector_offset) = %d\n", (char *)(freeblockvector + freeblockvector_offset));
    bcopy((char *)(freeblockvector + freeblockvector_offset), block.data, FDISK_FILESYSTEM_BLOCKSIZE);
    diskblocksize_returned = FdiskWriteBlock(i, &block);
  }
  
  // 6) Finally, setup superblock as valid filesystem and write superblock and boot record to disk: 
  // 6.1) boot record is all zeros in the first physical block, 
  bzero((char *)&block, FDISK_FILESYSTEM_BLOCKSIZE);
  // 6.2) superblock structure goes into the second physical block
  // bcopy ....
  sb.valid = 1;
  sb.dfs_blocksize = FDISK_FILESYSTEM_BLOCKSIZE;
  sb.num_dfs_blocks = FDISK_FILESYSTEM_NUM_BLOCKS;
  sb.inode_array_start_dfs_block_num = FDISK_INODE_BLOCK_START;
  sb.num_inode = FDISK_NUM_INODES;
  sb.free_block_vector_start_dfs_block_num = FDISK_FBV_BLOCK_START;
  bcopy((char *)&sb, (block.data + diskblocksize), sizeof(sb));
  //  6.3) Write superblock to the disk
  diskblocksize_returned = FdiskWriteBlock(FDISK_BOOT_FILESYSTEM_BLOCKNUM, &block);
/*
  for(i=512; i<(512 + sizeof(sb)); i++)
  {
    Printf("block.data[%d] = %d\n", i, block.data[i]);
  }
*/
  Printf("fdisk (%d): Formatted DFS disk for %d bytes.\n", getpid(), disksize);
}

int FdiskWriteBlock(uint32 blocknum, dfs_block *b) 
{
  // STUDENT: put your code here
  // Remember, the argument blocknum passed is dfs_blocknum not physical block.

  int i;
  uint32 disk_blocknum;
  char *disk_block_data_ptr;
  int diskblocksize_returned;
  int fail;

  //Printf("Inside fdisk: FdiskWriteBlock: blocknum = %d\n", blocknum);

  fail = 0;
  diskblocksize_returned = -1;
  disk_blocknum = (blocknum * (FDISK_FILESYSTEM_BLOCKSIZE / diskblocksize));

  for (i=0; i<(FDISK_FILESYSTEM_BLOCKSIZE / diskblocksize); i++, disk_blocknum++)
  {
    //Printf("Inside fdisk: FdiskWriteBlock: disk_blocknum = %d\n", disk_blocknum);
    disk_block_data_ptr = (b->data + (i * diskblocksize));
    diskblocksize_returned = disk_write_block(disk_blocknum, disk_block_data_ptr);
    if (diskblocksize_returned != diskblocksize)
    { 
      fail = 1;
      break;
    }
  }
  
  if (fail == 1)
  {
    Printf("ERROR: Inside fdisk: FdiskWriteBlock: diskblocksize_returned = %d\n", diskblocksize_returned);
  }
  else
  {
    //Printf("Inside fdisk: FdiskWriteBlock: diskblocksize_returned = %d\n", diskblocksize_returned);
  }  
  return diskblocksize_returned;
}

/*
void test_bcopy ()
{
  for(i=0; i<(sizeof(sb)/4); i++)
  {
    Printf("((int *)&sb + %d) = %d, *((int *)&sb + %d) = %d\n", i, (int)((int *)&sb + i), i, *((int *)&sb + i));
  }

  sb.valid = 0;
  sb.dfs_blocksize = 0;
  sb.num_dfs_blocks = 0;
  sb.inode_array_start_dfs_block_num = 0;
  sb.num_inode = 0;
  sb.free_block_vector_start_dfs_block_num = 0;

  for(i=0; i<(sizeof(sb)/4); i++)
  {
    Printf("((int *)&sb + %d) = %d, *((int *)&sb + %d) = %d\n", i, (int)((int *)&sb + i), i, *((int *)&sb + i));
  }

  bcopy((block.data + diskblocksize), (char *)&sb, sizeof(sb));

  for(i=0; i<(sizeof(sb)/4); i++)
  {
    Printf("((int *)&sb + %d) = %d, *((int *)&sb + %d) = %d\n", i, (int)((int *)&sb + i), i, *((int *)&sb + i));
  }
 
  Printf("block.data = %d, diskblocksize = %d, block.data + diskblocksize = %d\n", (int)block.data, diskblocksize, (int)(block.data + diskblocksize));
  for(i=0; i<sizeof(sb); i++)
  {
    Printf("((char *)&sb + %d) = %d, *((char *)&sb + %d) = %d\n", i, (int)((char *)&sb + i), i, *((char *)&sb + i));
  }

  for(i=0; i<(sizeof(sb)/4); i++)
  {
    Printf("((int *)&sb + %d) = %d, *((int *)&sb + %d) = %d\n", i, (int)((int *)&sb + i), i, *((int *)&sb + i));
  }

  bcopy((char *)&sb, (block.data), sizeof(sb));

  for(i=0; i<sizeof(sb); i++)
  {
    Printf("block.data[%d] = %d\n", i, block.data[i]);
  }
}
*/
