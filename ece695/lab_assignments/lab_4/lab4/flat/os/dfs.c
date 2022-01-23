#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "queue.h"
#include "disk.h"
#include "dfs.h"
#include "synch.h"

static dfs_superblock sb; // superblock
static uint32 fbv[DFS_FBV_MAX_NUM_WORDS]; // Free block vector
static dfs_inode inodes[DFS_INODE_MAX_NUM]; // all inodes

static dfs_block buffer_cache_data[DFS_BUFFER_CACHE_NUM_SLOTS]; // Buffer Cache data array
static buffer_cache_node buffer_cache_tag[DFS_BUFFER_CACHE_NUM_SLOTS];

static uint32 negativeone = 0xFFFFFFFF;
static inline uint32 invert(uint32 n) { return n ^ negativeone; }

// You have already been told about the most likely places where you should use locks. You may use 
// additional locks if it is really necessary.

// STUDENT: put your DFS level functions below.
// Some skeletons are provided. You can implement additional functions.


//-----------------------------------------------------------------
// DfsInavlidate marks the current version of the filesystem in
// memory as invalid.  This is really only useful when formatting
// the disk, to prevent the current memory version from overwriting
// what you already have on the disk when the OS exits.
//-----------------------------------------------------------------

uint32 read_fbv (int index)
{
  uint32 fbv_val;
  int intrs;

  dbprintf ('d', "Entering read_fbv\n");

  intrs = DisableIntrs ();
  fbv_val = fbv[index];
  RestoreIntrs (intrs);

  dbprintf ('d', "Leaving read_fbv\n");

  return fbv_val;
}

int read_sb_valid ()
{
  int sb_valid;
  int intrs;

  dbprintf ('d', "Entering read_sb_valid\n");

  intrs = DisableIntrs ();
  sb_valid = sb.valid;
  RestoreIntrs (intrs);

  dbprintf ('d', "Leaving read_sb_valid\n");

  return sb_valid;
}

void DfsInvalidate() 
{
  int intrs;

  dbprintf ('d', "Entering DfsInvalidate\n");

// This is just a one-line function which sets the valid bit of the 
// superblock to 0.
  intrs = DisableIntrs ();
  sb.valid = 0;
  RestoreIntrs (intrs);

  dbprintf ('d', "Leaving DfsInvalidate\n");
}


//-------------------------------------------------------------------
// DfsOpenFileSystem loads the file system metadata from the disk
// into memory.  Returns DFS_SUCCESS on success, and DFS_FAIL on 
// failure.
//-------------------------------------------------------------------

int DfsOpenFileSystem() 
{
  int i, j, k;
  disk_block db;
  int disk_block_size_returned;
  int disk_blocknum;
  int num_inode_dfs_blocks;
  int num_inodes_in_single_disk_block;
  int num_inodes_in_single_dfs_block;
  int inode_index;
  int num_fbv_dfs_blocks;
  int num_fbvs_in_single_disk_block;
  int num_fbvs_in_single_dfs_block;
  int fbv_index;
  int intrs;
  
  dbprintf ('d', "Entering DfsOpenFileSystem\n");

//Basic steps:
// 1) Check that filesystem is not already open
  if (read_sb_valid() == 1)
  {
    printf("ERROR: Inside DfsOpenFileSystem: filesystem is already opened\n");
    return DFS_FAIL;
  }

// 2) Read superblock from disk.  Note this is using the disk read rather 
// than the DFS read function because the DFS read requires a valid 
// filesystem in memory already, and the filesystem cannot be valid 
// until we read the superblock. Also, we don't know the block size 
// until we read the superblock, either. Use (DiskReadBlock function).
  disk_block_size_returned = DiskReadBlock(DFS_SUPERBLOCK_PHYSICAL_BLOCKNUM, &db);
  if (disk_block_size_returned != DISK_BLOCKSIZE)
  {
    printf("ERROR: Inside DfsOpenFileSystem: DiskReadBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
    return DFS_FAIL;
  }
  

// 3) Copy the data from the block we just read into the superblock in
// memory. Use bcopy
  bcopy (db.data, (char *)&sb, sizeof(sb));


// 4) All other blocks are sized by virtual block size:
// 4.1) Read inodes (Use DfsReadContiguousBytes)
// 4.2) Read free block vector
  
  num_inode_dfs_blocks = ((sb.num_inode * sizeof(dfs_inode)) / sb.dfs_blocksize);
  num_inodes_in_single_dfs_block = (sb.dfs_blocksize / sizeof(dfs_inode));
  num_inodes_in_single_disk_block = (DISK_BLOCKSIZE / sizeof(dfs_inode));
  for (i=(sb.inode_array_start_dfs_block_num); i<(sb.inode_array_start_dfs_block_num + num_inode_dfs_blocks); i++)
  {
    disk_blocknum = (i * (sb.dfs_blocksize / DISK_BLOCKSIZE));
    for (j=0; j<(sb.dfs_blocksize / DISK_BLOCKSIZE); j++, disk_blocknum++)
    {
      disk_blocknum = (disk_blocknum + j);
      disk_block_size_returned = DiskReadBlock(disk_blocknum, &db);
      if (disk_block_size_returned != DISK_BLOCKSIZE)
      {
        printf("ERROR: Inside DfsOpenFileSystem: DiskReadBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
        return DFS_FAIL;
      }
      for (k=0; k<num_inodes_in_single_disk_block; k++)
      {
        inode_index = ((i - sb.inode_array_start_dfs_block_num) * num_inodes_in_single_dfs_block) + (j * num_inodes_in_single_disk_block) + (k);

        intrs = DisableIntrs ();
        bcopy ((db.data + (k * sizeof(dfs_inode))), (char *)(&inodes[inode_index]), sizeof(dfs_inode));
        RestoreIntrs (intrs);
      }
    }
  }
  
  num_fbv_dfs_blocks = (sb.num_dfs_blocks / (sb.dfs_blocksize * DFS_BITS_PER_BYTE));
  num_fbvs_in_single_dfs_block = (sb.dfs_blocksize / DFS_FBV_BYTES_PER_ENTRY);
  num_fbvs_in_single_disk_block = (DISK_BLOCKSIZE / DFS_FBV_BYTES_PER_ENTRY);
  for (i=(sb.free_block_vector_start_dfs_block_num); i<(sb.free_block_vector_start_dfs_block_num + num_fbv_dfs_blocks); i++)
  {
    disk_blocknum = (i * (sb.dfs_blocksize / DISK_BLOCKSIZE));
    for (j=0; j<(sb.dfs_blocksize / DISK_BLOCKSIZE); j++, disk_blocknum++)
    {
      disk_blocknum = (disk_blocknum + j);
      disk_block_size_returned = DiskReadBlock(disk_blocknum, &db);
      if (disk_block_size_returned != DISK_BLOCKSIZE)
      {
        printf("ERROR: Inside DfsOpenFileSystem: DiskReadBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
        return DFS_FAIL;
      }
      for (k=0; k<num_fbvs_in_single_disk_block; k++)
      {
        fbv_index = ((i - sb.free_block_vector_start_dfs_block_num) * num_fbvs_in_single_dfs_block) + (j * num_fbvs_in_single_disk_block) + (k);

        intrs = DisableIntrs ();
        bcopy ((db.data + (k * DFS_FBV_BYTES_PER_ENTRY)), (char *)(&fbv[fbv_index]), DFS_FBV_BYTES_PER_ENTRY);
        RestoreIntrs (intrs);
      }
    }
  }

  
// 5) (5.1) Change superblock to be invalid, (5.2) write back to disk, then
// (5.3) change it back to be valid in memory (mysuperblock.valid = VALID)
  DfsInvalidate();
  bcopy((char *)&sb, db.data, sizeof(sb));
  disk_block_size_returned = DiskWriteBlock(DFS_SUPERBLOCK_PHYSICAL_BLOCKNUM, &db);
  if (disk_block_size_returned != DISK_BLOCKSIZE)
  {
    printf("ERROR: Inside DfsOpenFileSystem: DiskWriteBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
    return DFS_FAIL;
  }
  intrs = DisableIntrs ();
  sb.valid = 1;
  RestoreIntrs (intrs);

  dbprintf ('d', "Leaving DfsOpenFileSystem\n");
  return DFS_SUCCESS;  
}


//-----------------------------------------------------------------
// DfsModuleInit is called at boot time to initialize things and
// open the file system for use.
//-----------------------------------------------------------------

void DfsModuleInit() 
{
  int retrun_val;

  dbprintf ('d', "Entering DfsModuleInit\n");

// You essentially set the file system as invalid and then open 
// using DfsOpenFileSystem().
  DfsInvalidate();

  retrun_val = DfsOpenFileSystem();  
  if (retrun_val == DFS_FAIL)
  {
    printf("ERROR: Inside DfsModuleInit. DfsOpenFileSystem FAILED\n");
  }

// Initialize File Descriptors
  FileModuleInit();

// later initialize buffer cache-here.

  dbprintf ('d', "Leaving DfsModuleInit\n");
}


//-------------------------------------------------------------------
// DfsCloseFileSystem writes the current memory version of the
// filesystem metadata to the disk, and invalidates the memory's 
// version.
//-------------------------------------------------------------------

int DfsCloseFileSystem() 
{
  int i, j, k;
  disk_block db;
  int disk_block_size_returned;
  int disk_blocknum;
  int num_inode_dfs_blocks;
  int num_inodes_in_single_disk_block;
  int num_inodes_in_single_dfs_block;
  int inode_index;
  int num_fbv_dfs_blocks;
  int num_fbvs_in_single_disk_block;
  int num_fbvs_in_single_dfs_block;
  int fbv_index;
  int return_val;
  int intrs;
  
  dbprintf ('d', "Entering DfsCloseFileSystem\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsCloseFileSystem: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Writing Superblock to the disk
  //bzero((char *)&db, DISK_BLOCKSIZE);
  bzero(db.data, DISK_BLOCKSIZE);
  bcopy((char *)&sb, db.data, sizeof(sb));
  disk_block_size_returned = DiskWriteBlock(DFS_SUPERBLOCK_PHYSICAL_BLOCKNUM, &db);
  if (disk_block_size_returned != DISK_BLOCKSIZE)
  {
    printf("ERROR: Inside DfsCloseFileSystem: DiskWriteBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
    return DFS_FAIL;
  }
  
// 3) Writing inode array to the disk
  num_inode_dfs_blocks = ((sb.num_inode * sizeof(dfs_inode)) / sb.dfs_blocksize);
  num_inodes_in_single_dfs_block = (sb.dfs_blocksize / sizeof(dfs_inode));
  num_inodes_in_single_disk_block = (DISK_BLOCKSIZE / sizeof(dfs_inode));
  for (i=(sb.inode_array_start_dfs_block_num); i<(sb.inode_array_start_dfs_block_num + num_inode_dfs_blocks); i++)
  {
    disk_blocknum = (i * (sb.dfs_blocksize / DISK_BLOCKSIZE));
    for (j=0; j<(sb.dfs_blocksize / DISK_BLOCKSIZE); j++, disk_blocknum++)
    {
      for (k=0; k<num_inodes_in_single_disk_block; k++)
      {
        inode_index = ((i - sb.inode_array_start_dfs_block_num) * num_inodes_in_single_dfs_block) + (j * num_inodes_in_single_disk_block) + (k);
        intrs = DisableIntrs ();
        bcopy ((char *)(&inodes[inode_index]), (db.data + (k * sizeof(dfs_inode))), sizeof(dfs_inode));
        RestoreIntrs (intrs);
      }
      disk_block_size_returned = DiskWriteBlock(disk_blocknum, &db);
      if (disk_block_size_returned != DISK_BLOCKSIZE)
      {
        printf("ERROR: Inside DfsCloseFileSystem: DiskWriteBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
        return DFS_FAIL;
      }
    }
  }
  
// 4) Writing free block vector array to the disk
  num_fbv_dfs_blocks = (sb.num_dfs_blocks / (sb.dfs_blocksize * DFS_BITS_PER_BYTE));
  num_fbvs_in_single_dfs_block = (sb.dfs_blocksize / DFS_FBV_BYTES_PER_ENTRY);
  num_fbvs_in_single_disk_block = (DISK_BLOCKSIZE / DFS_FBV_BYTES_PER_ENTRY);
  for (i=(sb.free_block_vector_start_dfs_block_num); i<(sb.free_block_vector_start_dfs_block_num + num_fbv_dfs_blocks); i++)
  {
    disk_blocknum = (i * (sb.dfs_blocksize / DISK_BLOCKSIZE));
    for (j=0; j<(sb.dfs_blocksize / DISK_BLOCKSIZE); j++, disk_blocknum++)
    {
      for (k=0; k<num_fbvs_in_single_disk_block; k++)
      {
        fbv_index = ((i - sb.free_block_vector_start_dfs_block_num) * num_fbvs_in_single_dfs_block) + (j * num_fbvs_in_single_disk_block) + (k);
        intrs = DisableIntrs ();
        bcopy ((char *)(&fbv[fbv_index]), (db.data + (k * DFS_FBV_BYTES_PER_ENTRY)), DFS_FBV_BYTES_PER_ENTRY);
        RestoreIntrs (intrs);
      }
      disk_block_size_returned = DiskWriteBlock(disk_blocknum, &db);
      if (disk_block_size_returned != DISK_BLOCKSIZE)
      {
        printf("ERROR: Inside DfsCloseFileSystem: DiskWriteBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
        return DFS_FAIL;
      }
    }
  }

// 5) Flushing the Buffer Cache 
  return_val = DfsCacheFlush();
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside DfsCloseFileSystem: DfsCacheFlush FAILED.\n");
    return DFS_FAIL;
  }

// 6) Invalidating the memory's version of filesystem
  DfsInvalidate();
  
  dbprintf ('d', "Leaving DfsCloseFileSystem\n");
  return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsAllocateBlock allocates a DFS block for use. Remember to use 
// locks where necessary.
//-----------------------------------------------------------------

uint32 DfsAllocateBlock() 
{
  int i, j;
  int hit;
  int intrs;
  uint32 free_dfs_block_num;

  dbprintf ('d', "Entering DfsAllocateBlock\n");

// Check that file system has been validly loaded into memory
// Find the first free block using the free block vector (FBV), mark it in use
// Return handle to block

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsAllocateBlock: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Finding first free block using FBV and marking it "used"
  hit = 0;
  for (i=0; i<DFS_FBV_MAX_NUM_WORDS; i++)
  {
    for (j=0; j<DFS_FBV_BITS_PER_ENTRY; j++)
    {
      if ((read_fbv(i) & (1 << j)) == 0x00000000)
      {
        hit = 1;

        intrs = DisableIntrs ();    
        fbv[i] = fbv[i] | (1 << j);
        RestoreIntrs (intrs);

        free_dfs_block_num = ((i * DFS_FBV_BITS_PER_ENTRY) + j);
        break;
      }
    }
    if (hit == 1)
    {
      break;
    }
  }

  if (hit == 0)
  {
    printf("ERROR: Inside DfsAllocateBlock: block cannot be allocated. No Free blocks\n");
    return DFS_FAIL;
  }
  else
  {
    dbprintf ('d', "Leaving DfsAllocateBlock\n");
    return free_dfs_block_num;
  }
}


//-----------------------------------------------------------------
// DfsFreeBlock deallocates a DFS block.
//-----------------------------------------------------------------

int DfsFreeBlock(uint32 blocknum) 
{
  int bit_num;
  int fbv_index;
  uint32 bit_mask;
  int intrs;
  
  dbprintf ('d', "Entering DfsFreeBlock\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsFreeBlock: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Finding the blocknum in FBV and marking it "free"
  fbv_index = (blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (blocknum % DFS_FBV_BITS_PER_ENTRY);
  
  bit_mask = (1 << bit_num);
  bit_mask = invert (bit_mask);

  intrs = DisableIntrs ();
  fbv[fbv_index] = (fbv[fbv_index] & bit_mask);
  RestoreIntrs (intrs);

  dbprintf ('d', "Leaving DfsFreeBlock\n");
  return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsReadBlock reads an allocated DFS block from the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to read from it.  Returns DFS_FAIL
// on failure, and the number of bytes read on success.  
//-----------------------------------------------------------------

int DfsReadBlock(uint32 blocknum, dfs_block *b) 
{
  int i;
  int bit_num;
  int fbv_index;
  disk_block db;
  uint32 disk_blocknum;
  int disk_block_size_returned;

  dbprintf ('d', "Entering DfsReadBlock\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsReadBlock: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Checking dfs_block to be allocated
  fbv_index = (blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (blocknum % DFS_FBV_BITS_PER_ENTRY);
  if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000)
  {
    printf("ERROR: Inside DfsReadBlock: blocknum is not allocated. blocknum = %d\n", blocknum);
    return DFS_FAIL;
  }

// 3) Reading the (set of) block(s) from the disk
  //dbprintf ('d', "Inside DfsWriteBlock: blocknum = %d\n", blocknum);
  disk_blocknum = (blocknum * (sb.dfs_blocksize / DISK_BLOCKSIZE));
  for (i=0; i<(sb.dfs_blocksize / DISK_BLOCKSIZE); i++, disk_blocknum++)
  {
    //dbprintf ('d', "Inside DfsWriteBlock: i = %d\n", i);
    //dbprintf ('d', "Inside DfsWriteBlock: disk_blocknum = %d\n", disk_blocknum);

    disk_block_size_returned = DiskReadBlock(disk_blocknum, &db);
    if (disk_block_size_returned != DISK_BLOCKSIZE)
    {
      printf("ERROR: Inside DfsReadBlock: DiskReadBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
      return DFS_FAIL;
    }
    bcopy ((db.data), (b->data + (i * DISK_BLOCKSIZE)), DISK_BLOCKSIZE);
  }

  dbprintf ('d', "Leaving DfsReadBlock\n");
  return (sb.dfs_blocksize);
}


//-----------------------------------------------------------------
// DfsWriteBlock writes to an allocated DFS block on the disk
// (which could span multiple physical disk blocks).  The block
// must be allocated in order to write to it.  Returns DFS_FAIL
// on failure, and the number of bytes written on success.  
//-----------------------------------------------------------------

int DfsWriteBlock(uint32 blocknum, dfs_block *b)
{
  int i, j;
  int bit_num;
  int fbv_index;
  disk_block db;
  uint32 disk_blocknum;
  int disk_block_size_returned;

  dbprintf ('d', "Entering DfsWriteBlock\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsWriteBlock: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Checking dfs_block to be allocated
  fbv_index = (blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (blocknum % DFS_FBV_BITS_PER_ENTRY);
  if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000)
  {
    printf("ERROR: Inside DfsWriteBlock: blocknum is not allocated. blocknum = %d\n", blocknum);
    return DFS_FAIL;
  }

// 3) Writing the (set of) block(s) to the disk
  //dbprintf ('d', "Inside DfsWriteBlock: blocknum = %d\n", blocknum);
  disk_blocknum = (blocknum * (sb.dfs_blocksize / DISK_BLOCKSIZE));
  for (i=0; i<(sb.dfs_blocksize / DISK_BLOCKSIZE); i++, disk_blocknum++)
  {
    //dbprintf ('d', "Inside DfsWriteBlock: i = %d\n", i);
    dbprintf ('d', "Inside DfsWriteBlock: disk_blocknum = %d\n", disk_blocknum);
    
    bcopy ((b->data + (i * DISK_BLOCKSIZE)), (db.data), DISK_BLOCKSIZE);
/*
    for (j=0; j<DISK_BLOCKSIZE; j++)
    {
      dbprintf ('d', "Inside DfsWriteBlock: db.data[%d] = %d\n", j, db.data[j]);
    }
*/   
    disk_block_size_returned = DiskWriteBlock(disk_blocknum, &db);
    if (disk_block_size_returned != DISK_BLOCKSIZE)
    {
      printf("ERROR: Inside DfsWriteBlock: DiskWriteBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
      return DFS_FAIL;
    }
  }

  dbprintf ('d', "Leaving DfsWriteBlock\n");
  return (sb.dfs_blocksize);
}


////////////////////////////////////////////////////////////////////////////////
// Inode-based functions
////////////////////////////////////////////////////////////////////////////////

void print_inode (int handle)
{
  int i;
  int intrs;

  dbprintf ('d', "Entering print_inode\n");

  for(i=0; i<(sizeof(inodes[handle])); i++)
  {
    intrs = DisableIntrs ();
    //printf("((int *)&inodes[handle] + %d) = %d, *((int *)&inodes[handle] + %d) = %d\n", i, (int)((int *)&inodes[handle] + i), i, *((int *)&inodes[handle] + i));
    printf("((char *)&inodes[handle] + %d) = %d, *((char *)&inodes[handle] + %d) = %d\n", i, (int)((char *)&inodes[handle] + i), i, *((char *)&inodes[handle] + i));
    RestoreIntrs (intrs);
  }

  dbprintf ('d', "Leaving print_inode \n");
}

int read_inode_inuse (int inode_handle)
{
  int inuse;
  int intrs;

  //dbprintf ('d', "Entering read_inode_inuse\n");

  intrs = DisableIntrs ();
  inuse = inodes[inode_handle].inuse;
  RestoreIntrs (intrs);

  //dbprintf ('d', "Leaving read_inode_inuse\n");

  return inuse;
}

//-----------------------------------------------------------------
// DfsInodeFilenameExists looks through all the inuse inodes for 
// the given filename. If the filename is found, return the handle 
// of the inode. If it is not found, return DFS_FAIL.
//-----------------------------------------------------------------

int compare_filename (char *input_filename, char *inode_filename)
{
  int i;
  int mismatch;

  //dbprintf ('d', "Entering compare_filename\n");

  mismatch = 0;

  for (i=0; input_filename[i]!='\0'; i++) 
  {
    if (inode_filename[i] != input_filename[i])
    {
      mismatch = 1;
      break;
    }
  }
  
  if (mismatch == 1)
  {
    //dbprintf ('d', "Leaving compare_filename, with Error\n");
    return DFS_FAIL;
  }
  else
  {
    //dbprintf ('d', "Leaving compare_filename, with Success\n");
    return DFS_SUCCESS;
  }
}

uint32 DfsInodeFilenameExists(char *filename)
{
  int i;
  int hit;
  int inode_handle;
  int compare_filename_result;
  int intrs;

  dbprintf ('d', "Entering DfsInodeFilenameExists\n");

  hit = 0;
  inode_handle = DFS_FAIL;

  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeFilenameExists: filesystem is not opened\n");
    return DFS_FAIL;
  }

  for (i=0; i<DFS_INODE_MAX_NUM; i++)
  {
    if (read_inode_inuse(i) == 1)
    {
      intrs = DisableIntrs ();    
      compare_filename_result = compare_filename(filename, inodes[i].filename);
      RestoreIntrs (intrs);

      if (compare_filename_result == DFS_SUCCESS)
      {
        hit = 1;
        inode_handle = i;
        break;
      }
    }
  }
  
  if (hit == 1)
  {
    dbprintf ('d', "Leaving DfsInodeFilenameExists: File already present at inode_handle = %d\n", inode_handle);
    return inode_handle;
  }
  else
  {
    dbprintf ('d', "Leaving DfsInodeFilenameExists: File NOT present in inode\n");
    return DFS_FAIL;
  }
}


//-----------------------------------------------------------------
// DfsInodeOpen: search the list of all inuse inodes for the 
// specified filename. If the filename exists, return the handle 
// of the inode. If it does not, allocate a new inode for this 
// filename and return its handle. Return DFS_FAIL on failure. 
// Remember to use locks whenever you allocate a new inode.
//-----------------------------------------------------------------

void copy_filename_to_inode (char *input_filename, char *inode_filename)
{
  int i;

  dbprintf ('d', "Entering copy_filename_to_inode\n");

  for (i=0; i<DFS_INODE_MAX_FILENAME_LENGTH; i++) 
  {
    inode_filename[i] = 0;
  }
  
  for (i=0; input_filename[i]!='\0'; i++) 
  {
    inode_filename[i] = input_filename[i];
  }  

  dbprintf ('d', "Leaving copy_filename_to_inode\n");
}

void initialize_inode (int handle, char *filename)
{
  int i;
  int intrs;

  dbprintf ('d', "Entering initialize_inode\n");

  intrs = DisableIntrs ();    
  copy_filename_to_inode(filename, inodes[handle].filename);
  RestoreIntrs (intrs);

  inodes[handle].current_file_size = 0x00000000;

  for (i=0; i<DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS; i++)
  {
    inodes[handle].direct_addressed_blocks[i] = 0x00000000;
  }

  inodes[handle].single_indirect_addressed_blocks_dfs_block_num = 0x00000000;
  inodes[handle].double_indirect_addressed_blocks_dfs_block_num = 0x00000000; 
  
  //print_inode(handle);
 
  dbprintf ('d', "Leaving initialize_inode\n");
}

uint32 DfsInodeOpen(char *filename) 
{
  int i;
  int search_hit;
  int allocate_hit;
  int search_inode_handle;
  int allocate_inode_handle;
  int intrs;
  int compare_filename_result;

  dbprintf ('d', "Entering DfsInodeOpen\n");

  search_hit = 0;
  search_inode_handle = DFS_FAIL;
  allocate_hit = 0;
  allocate_inode_handle = DFS_FAIL;

  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeOpen: filesystem is not opened\n");
    return DFS_FAIL;
  }
 
  for (i=0; i<DFS_INODE_MAX_NUM; i++)
  {
    if (read_inode_inuse(i) == 1)
    {
      intrs = DisableIntrs ();    
      compare_filename_result = compare_filename(filename, inodes[i].filename);
      RestoreIntrs (intrs);

      if (compare_filename_result == DFS_SUCCESS)
      {
        search_hit = 1;
        search_inode_handle = i;
        break;
      }
    }
  }
  
  if (search_hit == 1)
  {
    dbprintf ('d', "Leaving DfsInodeOpen: Filename existed at inode = %d\n", search_inode_handle);
    return search_inode_handle;
  }

  for (i=0; i<DFS_INODE_MAX_NUM; i++)
  {
    if (read_inode_inuse(i) == 0)
    {
      allocate_hit = 1;
      allocate_inode_handle = i;

      initialize_inode(i, filename);

      intrs = DisableIntrs ();    
      inodes[i].inuse = 1;
      RestoreIntrs (intrs);

      break;
    }
  }

  if (allocate_hit == 1)
  {
    dbprintf ('d', "Leaving DfsInodeOpen. Allocated new inode = %d\n", allocate_inode_handle);
    return allocate_inode_handle;
  }
  else
  {
    printf("ERROR: Inside DfsInodeOpen\n");
    return DFS_FAIL;
  }
}


//-----------------------------------------------------------------
// DfsInodeDelete de-allocates any data blocks used by this inode, 
// including the indirect addressing block if necessary, then mark 
// the inode as no longer in use. Use locks when modifying the 
// "inuse" flag in an inode.Return DFS_FAIL on failure, and 
// DFS_SUCCESS on success.
//-----------------------------------------------------------------

int check_and_deallocate_block (int blocknum)
{
  int bit_num;
  int fbv_index;
  int return_val;

  //dbprintf ('d', "Entering check_and_deallocate_block\n");

  fbv_index = (blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (blocknum % DFS_FBV_BITS_PER_ENTRY);
  
  if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000)
  {
    //dbprintf ('d', "Inside check_and_deallocate_block: Block already free, no need to do anything: Freed blocknum = %d\n", blocknum);
  }
  else
  {
    return_val = DfsFreeBlock(blocknum);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_deallocate_block: blocknum cannot be de-allocated. blocknum = %d\n", blocknum);
      return DFS_FAIL;
    }
  }

  //dbprintf ('d', "Leaving check_and_deallocate_block: Freed blocknum = %d\n", blocknum);
  return DFS_SUCCESS;
}

int delete_single_indirect_addressed_data_blocks (int signle_indirect_blocknum)
{
  int i;
  int dfs_blocksize_returned;
  dfs_block dfsb;
  uint32 data_blocknum;
  int return_val;

  dbprintf ('d', "Entering delete_single_indirect_addressed_data_blocks\n");

  dfs_blocksize_returned = DfsReadBlock (signle_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside delete_single_indirect_addressed_data_blocks: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

  for (i=0; i<DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS; i++)
  {
    bcopy ((dfsb.data + (i * DFS_FBV_BYTES_PER_ENTRY)), (char *)&data_blocknum, DFS_FBV_BYTES_PER_ENTRY);
    
    if (data_blocknum == 0x00000000)
    {
      dbprintf ('d', "Inside delete_single_indirect_addressed_data_blocks: Block is in its default value, thus already free, no need to do anything\n");
    }
    else 
    {
      return_val = check_and_deallocate_block(data_blocknum);
      if (return_val == DFS_FAIL)
      {
        printf("ERROR: Inside delete_single_indirect_addressed_data_blocks: check_and_deallocate_block Failed\n");
        return DFS_FAIL;
      }
    }
  }

  return_val = check_and_deallocate_block(signle_indirect_blocknum);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside delete_single_indirect_addressed_data_blocks: check_and_deallocate_block Failed\n");
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving delete_single_indirect_addressed_data_blocks\n");
  return DFS_SUCCESS;
}

int delete_double_indirect_addressed_data_blocks (int double_indirect_blocknum)
{
  int i;
  int dfs_blocksize_returned;
  dfs_block dfsb;
  uint32 single_indirect_dfs_blocknum;
  int return_val;

  dbprintf ('d', "Entering delete_double_indirect_addressed_data_blocks\n");

  dfs_blocksize_returned = DfsReadBlock (double_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside delete_double_indirect_addressed_data_blocks: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

  for (i=0; i<DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS; i++)
  {
    bcopy ((dfsb.data + (i * DFS_FBV_BYTES_PER_ENTRY)), (char *)&single_indirect_dfs_blocknum, DFS_FBV_BYTES_PER_ENTRY);

    // Checking and deallocating single-indirect addressed dfs blocks within double-indirect addressing table
    if (single_indirect_dfs_blocknum == 0x00000000)
    {
      dbprintf ('d', "Inside delete_double_indirect_addressed_data_blocks: Single-Indirect addressed table is in its default value, thus already free, no need to do anything\n");
    }
    else 
    {
      return_val = delete_single_indirect_addressed_data_blocks(single_indirect_dfs_blocknum);
      if (return_val == DFS_FAIL)
      {
        printf("ERROR: Inside delete_double_indirect_addressed_data_blocks: delete_single_indirect_addressed_data_blocks Failed\n");
        return DFS_FAIL;
      }
    }
  }

  return_val = check_and_deallocate_block(double_indirect_blocknum);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside delete_single_indirect_addressed_data_blocks: check_and_deallocate_block Failed\n");
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving delete_double_indirect_addressed_data_blocks\n");
  return DFS_SUCCESS;
}

int DfsInodeDelete(uint32 handle) 
{
  int i;
  int intrs;
  int return_val;

  dbprintf ('d', "Entering DfsInodeDelete\n");

  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeDelete: filesystem is not opened\n");
    return DFS_FAIL;
  }

  if (read_inode_inuse(handle) == 0)
  {
    printf("ERROR: Inside DfsInodeDelete: inode pointed by handle has insue=0. handle = %d\n", handle);
    return DFS_FAIL;
  }

// Clearing the inuse field of inode
  intrs = DisableIntrs ();    
  inodes[handle].inuse = 0;
  RestoreIntrs (intrs);

// Checking and deallocating direct addressed dfs blocks
  for (i=0; i<DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS; i++)
  {
    if (inodes[handle].direct_addressed_blocks[i] == 0x00000000)
    {
      dbprintf ('d', "Inside DfsInodeDelete: Block is in its default value, thus already free, no need to do anything\n");
    }
    else 
    {
      return_val = check_and_deallocate_block(inodes[handle].direct_addressed_blocks[i]);
      if (return_val == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeDelete: check_and_deallocate_block Failed\n");
        return DFS_FAIL;
      }
    }
  }

// Checking and deallocating single-indirect addressed dfs blocks
  if (inodes[handle].single_indirect_addressed_blocks_dfs_block_num == 0x00000000)
  {
    dbprintf ('d', "Inside DfsInodeDelete: Single-Indirect addressed table is in its default value, thus already free, no need to do anything\n");
  }
  else 
  {
    return_val = delete_single_indirect_addressed_data_blocks(inodes[handle].single_indirect_addressed_blocks_dfs_block_num);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeDelete: delete_single_indirect_addressed_data_blocks Failed\n");
      return DFS_FAIL;
    }
  }

// Checking and deallocating double-indirect addressed dfs blocks
  if (inodes[handle].double_indirect_addressed_blocks_dfs_block_num == 0x00000000)
  {
    dbprintf ('d', "Inside DfsInodeDelete: Double-Indirect addressed table is in its default value, thus already free, no need to do anything\n");
  }
  else 
  {
    return_val = delete_double_indirect_addressed_data_blocks(inodes[handle].double_indirect_addressed_blocks_dfs_block_num);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeDelete: delete_double_indirect_addressed_data_blocks Failed\n");
      return DFS_FAIL;
    }
  }

  dbprintf ('d', "Leaving DfsInodeDelete\n");
  return DFS_SUCCESS;
}


//-----------------------------------------------------------------
// DfsInodeTranslateVirtualToFilesys translates the 
// virtual_blocknum to the corresponding file system block using 
// the inode identified by handle. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 get_dfs_blocknum_within_single_indirect_addressed_blocks (uint32 single_indirect_blocknum, uint32 virtual_blocknum)
{
  int dfs_blocksize_returned;
  dfs_block dfsb;
  int single_indirect_dfs_blockoffset;
  uint32 dfs_blocknum;

  dbprintf ('d', "Entering get_dfs_blocknum_within_single_indirect_addressed_blocks\n");

// Reading the Single-Indirect DFS Block
  dfs_blocksize_returned = DfsReadBlock (single_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside get_dfs_blocknum_within_single_indirect_addressed_blocks: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

// Getting the virtual_blocknum indexed dfs block from the Single-Indirect DFS Block
  single_indirect_dfs_blockoffset = ((virtual_blocknum - DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS) * DFS_BLOCK_POINTER_SIZE);
  bcopy ((dfsb.data + single_indirect_dfs_blockoffset), (char *)&dfs_blocknum, DFS_FBV_BYTES_PER_ENTRY);

  dbprintf ('d', "Leaving get_dfs_blocknum_within_single_indirect_addressed_blocks: single_indirect_dfs_blockoffset = %d, dfs_blocknum = %d\n", single_indirect_dfs_blockoffset, dfs_blocknum);
  return dfs_blocknum;
}

uint32 get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks (uint32 double_indirect_blocknum, uint32 virtual_blocknum)
{
  int dfs_blocksize_returned;
  dfs_block dfsb;
  int double_indirect_dfs_blockoffset;
  uint32 single_indirect_blocknum;

  dbprintf ('d', "Entering get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks\n");

// Reading the Double-Indirect DFS Block
  dfs_blocksize_returned = DfsReadBlock (double_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

// Getting the virtual_blocknum indexed Single-Indirect DFS Block Number from the Double-Indirect DFS Block
  double_indirect_dfs_blockoffset = (((virtual_blocknum - DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS - DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) / DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) * DFS_BLOCK_POINTER_SIZE);
  bcopy ((dfsb.data + double_indirect_dfs_blockoffset), (char *)&single_indirect_blocknum, DFS_FBV_BYTES_PER_ENTRY);

  dbprintf ('d', "Leaving get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks: double_indirect_dfs_blockoffset = %d, single_indirect_blocknum = %d\n", double_indirect_dfs_blockoffset, single_indirect_blocknum);
  return single_indirect_blocknum;
}

uint32 get_dfs_blocknum_within_double_indirect_addressed_blocks (uint32 double_indirect_blocknum, uint32 virtual_blocknum)
{
  int dfs_blocksize_returned;
  dfs_block dfsb;
  int single_indirect_dfs_blockoffset;
  uint32 single_indirect_blocknum;
  uint32 dfs_blocknum;

  dbprintf ('d', "Entering get_dfs_blocknum_within_double_indirect_addressed_blocks\n");

// Getting the virtual_blocknum indexed Single-Indirect DFS Block Number from the Double-Indirect DFS Block
  single_indirect_blocknum = get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks(double_indirect_blocknum, virtual_blocknum);
  if (single_indirect_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside get_dfs_blocknum_within_double_indirect_addressed_blocks: get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks FAILED.\n");
    return DFS_FAIL;
  }

// Reading the Single-Indirect DFS Block
  dfs_blocksize_returned = DfsReadBlock (single_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside get_dfs_blocknum_within_double_indirect_addressed_blocks: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

// Getting the virtual_blocknum indexed dfs block from the Single-Indirect DFS Block
  single_indirect_dfs_blockoffset = (((virtual_blocknum - DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS - DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) % DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) * DFS_BLOCK_POINTER_SIZE);
  bcopy ((dfsb.data + single_indirect_dfs_blockoffset), (char *)&dfs_blocknum, DFS_FBV_BYTES_PER_ENTRY);

  dbprintf ('d', "Leaving get_dfs_blocknum_within_double_indirect_addressed_blocks: single_indirect_dfs_blockoffset = %d, dfs_blocknum = %d\n", single_indirect_dfs_blockoffset, dfs_blocknum);
  return dfs_blocknum;
}

uint32 DfsInodeTranslateVirtualToFilesys(uint32 handle, uint32 virtual_blocknum) 
{
  uint32 dfs_blocknum;

  dbprintf ('d', "Entering DfsInodeTranslateVirtualToFilesys\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeTranslateVirtualToFilesys: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 1) Checking if the virtual block falls in direct addressing table. Returns the content of array
  if (virtual_blocknum < DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS)
  {
    dfs_blocknum = inodes[handle].direct_addressed_blocks[virtual_blocknum];
  }
// 2) Checking if the virtual block falls in single-indirect addressing table. Return the value of dfs blocknum from the single-indirect address dfs blocknum
  else if ((virtual_blocknum >= DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS) && (virtual_blocknum < (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)))
  {
    dfs_blocknum = get_dfs_blocknum_within_single_indirect_addressed_blocks(inodes[handle].single_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
    if (dfs_blocknum == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeTranslateVirtualToFilesys: get_dfs_blocknum_within_single_indirect_addressed_blocks Failed\n");
      return DFS_FAIL;
    }
  }
// 3) Checking if the virtual block falls in double-indirect addressing table. Get the single-indirect address blocknum. Return the value of dfs blocknum from the single-indirect address dfs blocknum
  else if ((virtual_blocknum >= (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)) && (virtual_blocknum < DFS_INODE_MAX_VIRTUAL_BLOCKNUM))
  {
    dfs_blocknum = get_dfs_blocknum_within_double_indirect_addressed_blocks(inodes[handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
    if (dfs_blocknum == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeTranslateVirtualToFilesys: get_dfs_blocknum_within_double_indirect_addressed_blocks Failed\n");
      return DFS_FAIL;
    }
  }
// 4) Return with ERROR, if virtual block exceeds the max limit
  else
  {
    printf("ERROR: Inside DfsInodeTranslateVirtualToFilesys: Virtual blocknum exceeded max limit. virtual_blocknum = %d\n", virtual_blocknum);
    return DFS_FAIL;
  }
  
  dbprintf ('d', "Leaving DfsInodeTranslateVirtualToFilesys: dfs_blocknum = %d\n", dfs_blocknum);
  return dfs_blocknum;
}


//-----------------------------------------------------------------
// DfsInodeAllocateVirtualBlock allocates a new filesystem block 
// for the given inode, storing its blocknumber at index 
// virtual_blocknumber in the translation table. If the 
// virtual_blocknumber resides in the indirect address space, and 
// there is not an allocated indirect addressing table, allocate it. 
// Return DFS_FAIL on failure, and the newly allocated file system 
// block number on success.
//-----------------------------------------------------------------

int check_and_allocate_block_for_inode_single_indirect_addressing_table (int handle)
{
  uint32 blocknum;
  int bit_num;
  int fbv_index;
  uint32 newly_allocated_dfs_block_num;

  dbprintf ('d', "Entering check_and_allocate_block_for_inode_single_indirect_addressing_table\n");

  blocknum = inodes[handle].single_indirect_addressed_blocks_dfs_block_num;
  fbv_index = (blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (blocknum % DFS_FBV_BITS_PER_ENTRY);

// Checking if the indirect addressing table has the default value. Allocate a new block and assign it in the inode
  if (blocknum == 0x00000000)
  {
    newly_allocated_dfs_block_num = DfsAllocateBlock();
    if (newly_allocated_dfs_block_num == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_allocate_block_for_inode_single_indirect_addressing_table: DfsAllocateBlock Failed. newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
      return DFS_FAIL;
    }
    inodes[handle].single_indirect_addressed_blocks_dfs_block_num = newly_allocated_dfs_block_num;
    dbprintf ('d', "Leaving check_and_allocate_block_for_inode_single_indirect_addressing_table: newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
    return DFS_SUCCESS;
  } 
// Checking if the indirect addressing table has has been assigned before, but not allocated. Return with Error
  else if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000) 
  {
    printf("ERROR: Inside check_and_allocate_block_for_inode_single_indirect_addressing_table: Inode Single-Indirect Addressing has been assigned before, but not allocated\n");
    return DFS_FAIL;
  }
// Checking if the indirect addressing table has has been assigned before and is used. No need to do anything
  else
  {
    dbprintf ('d', "Leaving check_and_allocate_block_for_inode_single_indirect_addressing_table: Block already allocated, no need to do anything\n");
    return DFS_SUCCESS;
  }
}

int write_dfs_blocknum_within_single_indirect_addressed_block (uint32 single_indirect_blocknum, uint32 virtual_blocknum, uint32 dfs_blocknum)
{
  int dfs_blocksize_returned;
  dfs_block dfsb;
  int single_indirect_dfs_blockoffset;

  dbprintf ('d', "Entering write_dfs_blocknum_within_single_indirect_addressed_block\n");

// Reading the Inode Single-Indirect Addressing DFS Block
  dfs_blocksize_returned = DfsReadBlock (single_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside write_dfs_blocknum_within_single_indirect_addressed_block: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

// Updating the dfs_blocknum for the virtual_blocknum indext in the Inode Single-Indirect Addressing DFS Block
  single_indirect_dfs_blockoffset = ((virtual_blocknum - DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS) * DFS_BLOCK_POINTER_SIZE);
  bcopy ((char *)&dfs_blocknum, (dfsb.data + single_indirect_dfs_blockoffset), DFS_FBV_BYTES_PER_ENTRY);

// Writing the updated value to the Inode Single-Indirect Addressing DFS Block
  dfs_blocksize_returned = DfsWriteBlock (single_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside write_dfs_blocknum_within_single_indirect_addressed_block: DfsWriteBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving write_dfs_blocknum_within_single_indirect_addressed_block\n");
  return DFS_SUCCESS;
}

int check_and_allocate_block_in_single_addressing_table (int inode_handle, uint32 virtual_blocknum, uint32 dfs_blocknum)
{
  int return_val;

  dbprintf ('d', "Entering check_and_allocate_block_in_single_addressing_table\n");

// Check and allocate the dfs block for Single-Indirect Addressed table in Inode
  return_val = check_and_allocate_block_for_inode_single_indirect_addressing_table(inode_handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside check_and_allocate_block_in_single_addressing_table: check_and_allocate_block_for_inode_single_indirect_addressing_table Failed\n");
    return DFS_FAIL;
  }

// After the block has been allocated for Single-Indirect Addressed table in Inode, update the the vlaue of virtual_blocknum indexed dfs_blocknum in Single-Indirect Addressed table in inode
  return_val = write_dfs_blocknum_within_single_indirect_addressed_block(inodes[inode_handle].single_indirect_addressed_blocks_dfs_block_num, virtual_blocknum, dfs_blocknum);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside check_and_allocate_block_in_single_addressing_table: write_dfs_blocknum_within_single_indirect_addressed_block Failed\n");
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving check_and_allocate_block_in_single_addressing_table\n");
  return DFS_SUCCESS;
}

int check_and_allocate_block_for_inode_double_indirect_addressing_table (int handle)
{
  uint32 blocknum;
  int bit_num;
  int fbv_index;
  uint32 newly_allocated_dfs_block_num;

  dbprintf ('d', "Entering check_and_allocate_block_for_inode_double_indirect_addressing_table\n");

  blocknum = inodes[handle].double_indirect_addressed_blocks_dfs_block_num;
  fbv_index = (blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (blocknum % DFS_FBV_BITS_PER_ENTRY);

// Checking if the indirect addressing table has the default value. Allocate a new block and assign it in the inode
  if (blocknum == 0x00000000)
  {
    newly_allocated_dfs_block_num = DfsAllocateBlock();
    if (newly_allocated_dfs_block_num == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_allocate_block_for_inode_double_indirect_addressing_table: DfsAllocateBlock Failed. newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
      return DFS_FAIL;
    }
    inodes[handle].double_indirect_addressed_blocks_dfs_block_num = newly_allocated_dfs_block_num;
    dbprintf ('d', "Leaving check_and_allocate_block_for_inode_double_indirect_addressing_table: newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
    return DFS_SUCCESS;
  } 
// Checking if the indirect addressing table has has been assigned before, but not allocated. Return with Error
  else if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000) 
  {
    printf("ERROR: Inside check_and_allocate_block_for_inode_double_indirect_addressing_table: Inode Double-Indirect Addressing has been assigned before, but not allocated\n");
    return DFS_FAIL;
  }
// Checking if the indirect addressing table has has been assigned before and is used. No need to do anything
  else
  {
    dbprintf ('d', "Leaving check_and_allocate_block_for_inode_double_indirect_addressing_table: Block already allocated, no need to do anything\n");
    return DFS_SUCCESS;
  }
}

int write_single_indirect_blocknum_within_double_indirect_addressed_block (uint32 double_indirect_blocknum, uint32 virtual_blocknum, uint32 single_indirect_blocknum)
{
  int dfs_blocksize_returned;
  dfs_block dfsb;
  int double_indirect_dfs_blockoffset;

  dbprintf ('d', "Entering write_single_indirect_blocknum_within_double_indirect_addressed_block\n");

// Reading the Inode Double-Indirect Addressing DFS Block
  dfs_blocksize_returned = DfsReadBlock (double_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside write_single_indirect_blocknum_within_double_indirect_addressed_block: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

// Updating the single_indirect_blocknum indexed in the Inode Double-Indirect Addressing DFS Block
  double_indirect_dfs_blockoffset = (((virtual_blocknum - DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS - DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) / DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) * DFS_BLOCK_POINTER_SIZE);
  bcopy ((char *)&single_indirect_blocknum, (dfsb.data + double_indirect_dfs_blockoffset), DFS_FBV_BYTES_PER_ENTRY);

// Writing the updated value to the Inode Double-Indirect Addressing DFS Block
  dfs_blocksize_returned = DfsWriteBlock (double_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside write_single_indirect_blocknum_within_double_indirect_addressed_block: DfsWriteBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving write_single_indirect_blocknum_within_double_indirect_addressed_block\n");
  return DFS_SUCCESS;
}

int check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table (uint32 double_indirect_blocknum , uint32 virtual_blocknum)
{
  uint32 single_indirect_blocknum;
  int bit_num;
  int fbv_index;
  uint32 newly_allocated_dfs_block_num;
  int return_val;

  dbprintf ('d', "Entering check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table\n");

  single_indirect_blocknum = get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks(double_indirect_blocknum, virtual_blocknum);
  if (single_indirect_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table: get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks FAILED.\n");
    return DFS_FAIL;
  }

  fbv_index = (single_indirect_blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (single_indirect_blocknum % DFS_FBV_BITS_PER_ENTRY);

// Checking if the indirect addressing table has the default value. Allocate a new block and assign it in the inode
  if (single_indirect_blocknum == 0x00000000)
  {
    newly_allocated_dfs_block_num = DfsAllocateBlock();
    if (newly_allocated_dfs_block_num == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table: DfsAllocateBlock Failed. newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
      return DFS_FAIL;
    }

    return_val = write_single_indirect_blocknum_within_double_indirect_addressed_block (double_indirect_blocknum, virtual_blocknum, newly_allocated_dfs_block_num);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table:  Failed\n");
      return DFS_FAIL;
    }

    dbprintf ('d', "Leaving check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table: newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
    return DFS_SUCCESS;
  } 
// Checking if the single-indirect addressing block within double-indirect addressed table has been NOT allocated
  else if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000) 
  {
    newly_allocated_dfs_block_num = DfsAllocateBlock();
    if (newly_allocated_dfs_block_num == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table: DfsAllocateBlock Failed. newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
      return DFS_FAIL;
    }

    return_val = write_single_indirect_blocknum_within_double_indirect_addressed_block (double_indirect_blocknum, virtual_blocknum, newly_allocated_dfs_block_num);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table:  Failed\n");
      return DFS_FAIL;
    }

    dbprintf ('d', "Leaving check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table: newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
    return DFS_SUCCESS;
  }
// Checking if the indirect addressing table has has been allocated before. No need to do anything
  else
  {
    dbprintf ('d', "Leaving check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table: Block already allocated, no need to do anything\n");
    return DFS_SUCCESS;
  }
}

int write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block (uint32 double_indirect_blocknum, uint32 virtual_blocknum, uint32 dfs_blocknum)
{
  int dfs_blocksize_returned;
  dfs_block dfsb;
  uint32 single_indirect_blocknum;
  int single_indirect_dfs_blockoffset;

  dbprintf ('d', "Entering write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block\n");

  single_indirect_blocknum = get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks(double_indirect_blocknum, virtual_blocknum);
  if (single_indirect_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block: get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks FAILED.\n");
    return DFS_FAIL;
  }

// Reading the Inode Single-Indirect Addressing DFS Block
  dfs_blocksize_returned = DfsReadBlock (single_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

// Updating the dfs_blocknum for the virtual_blocknum indext in the Inode Single-Indirect Addressing DFS Block
  single_indirect_dfs_blockoffset = (((virtual_blocknum - DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS - DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) % DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS) * DFS_BLOCK_POINTER_SIZE);
  bcopy ((char *)&dfs_blocknum, (dfsb.data + single_indirect_dfs_blockoffset), DFS_FBV_BYTES_PER_ENTRY);

// Writing the updated value to the Inode Single-Indirect Addressing DFS Block
  dfs_blocksize_returned = DfsWriteBlock (single_indirect_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block: DfsWriteBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block\n");
  return DFS_SUCCESS;
}

int check_and_allocate_block_in_double_addressing_table (int inode_handle, uint32 virtual_blocknum, uint32 dfs_blocknum)
{
  int return_val;

  dbprintf ('d', "Entering check_and_allocate_block_in_double_addressing_table\n");

// Check and allocate the dfs block for Double-Indirect Addressed table in Inode
  return_val = check_and_allocate_block_for_inode_double_indirect_addressing_table(inode_handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside check_and_allocate_block_in_double_addressing_table: check_and_allocate_block_for_inode_double_indirect_addressing_table Failed\n");
    return DFS_FAIL;
  }

// After the block has been allocated for Double-Indirect Addressed table in Inode, check whether the single-indirect addressed table is allocated in the double-indirect addressed block in inode
  return_val = check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table(inodes[inode_handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside check_and_allocate_block_in_double_addressing_table: check_and_allocate_block_for_single_indirect_in_double_indirect_addressing_table Failed\n");
    return DFS_FAIL;
  }

// Finally write the dfs_blocknum for the Virtual Block in Double-Indirect Addressed table in Inode
  return_val = write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block(inodes[inode_handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum, dfs_blocknum);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside check_and_allocate_block_in_double_addressing_table: write_dfs_blocknum_within_single_indirect_addressed_block_in_double_indirect_addressed_block Failed\n");
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving check_and_allocate_block_in_double_addressing_table\n");
  return DFS_SUCCESS;
}

uint32 DfsInodeAllocateVirtualBlock(uint32 handle, uint32 virtual_blocknum) 
{
  uint32 newly_allocated_dfs_block_num;
  int return_val;

  dbprintf ('d', "Entering DfsInodeAllocateVirtualBlock\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeAllocateVirtualBlock: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Allocating new dfs block for the virtual block
  newly_allocated_dfs_block_num = DfsAllocateBlock();
  if (newly_allocated_dfs_block_num == DFS_FAIL)
  {
    printf("ERROR: Inside DfsInodeAllocateVirtualBlock: New dfs block cannot be allocated. newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
    return DFS_FAIL;
  }
  
// 3) Updating direct addresses inode table for the virtual address
  if (virtual_blocknum < DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS)
  {
    inodes[handle].direct_addressed_blocks[virtual_blocknum] = newly_allocated_dfs_block_num;
  }
// 4) Updating single-indirect addresses inode table for the virtual address
  else if ((virtual_blocknum >= DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS) && (virtual_blocknum < (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)))
  {
    return_val = check_and_allocate_block_in_single_addressing_table (handle, virtual_blocknum, newly_allocated_dfs_block_num);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeAllocateVirtualBlock: check_and_allocate_block_in_single_addressing_table Failed\n");
      return DFS_FAIL;
    }
  }
// 5) Updating double-indirect addresses inode table for the virtual address
  else if ((virtual_blocknum >= (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)) && (virtual_blocknum < DFS_INODE_MAX_VIRTUAL_BLOCKNUM))
  {
    return_val = check_and_allocate_block_in_double_addressing_table(handle, virtual_blocknum, newly_allocated_dfs_block_num);
    if (return_val == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeAllocateVirtualBlock: check_and_allocate_block_in_double_addressing_table Failed\n");
      return DFS_FAIL;
    }
  }
  else
  {
    printf("ERROR: Inside DfsInodeAllocateVirtualBlock: Virtual blocknum exceeded max limit. virtual_blocknum = %d\n", virtual_blocknum);
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving DfsInodeAllocateVirtualBlock: newly_allocated_dfs_block_num = %d\n", newly_allocated_dfs_block_num);
  return newly_allocated_dfs_block_num;
}


//-----------------------------------------------------------------
// DfsInodeWriteBytes writes num_bytes from the memory pointed to 
// by mem to the file represented by the inode handle, starting at 
// virtual byte start_byte. Note that if you are only writing part 
// of a given file system block, you'll need to read that block 
// from the disk first. Return DFS_FAIL on failure and the number 
// of bytes written on success.
//-----------------------------------------------------------------

int check_block_in_single_indirect_addressed_table_to_be_allocated (uint32 handle, uint32 virtual_blocknum)
{
  uint32 dfs_blocknum;
  int bit_num;
  int fbv_index;
  
  dbprintf ('d', "Entering check_block_in_single_indirect_addressed_table_to_be_allocated\n");

  dfs_blocknum = get_dfs_blocknum_within_single_indirect_addressed_blocks (inodes[handle].single_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
  if (dfs_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside check_block_in_single_indirect_addressed_table_to_be_allocated: get_dfs_blocknum_within_single_indirect_addressed_blocks FAILED. dfs_blocknum = %d\n", dfs_blocknum);
    return DFS_FAIL;
  }
  
// Check the dfs_blocknum to be allocated
  fbv_index = (dfs_blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (dfs_blocknum % DFS_FBV_BITS_PER_ENTRY);

  if (dfs_blocknum == 0x00000000) 
  {
    dbprintf ('d', "Leaving check_block_in_single_indirect_addressed_table_to_be_allocated: dfs_blocknum has the default value and is NOT allocated\n");
    return DFS_FAIL;
  }
  else if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000) 
  {
    dbprintf ('d', "Leaving check_block_in_single_indirect_addressed_table_to_be_allocated: dfs_blocknum NOT allocated\n");
    return DFS_FAIL;
  }
  else 
  {
    dbprintf ('d', "Leaving check_block_in_single_indirect_addressed_table_to_be_allocated: dfs_blocknum IS allocated\n");
    return DFS_SUCCESS;
  }
}

int check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated (uint32 handle, uint32 virtual_blocknum)
{
  uint32 single_indirect_blocknum;
  int bit_num;
  int fbv_index;
  
  dbprintf ('d', "Entering check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated\n");

  single_indirect_blocknum = get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks(inodes[handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
  if (single_indirect_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated: get_single_indirect_addressed_blocknum_within_double_indirect_addressed_blocks FAILED.\n");
    return DFS_FAIL;
  }

// Check the single_indirect_blocknum to be allocated
  fbv_index = (single_indirect_blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (single_indirect_blocknum % DFS_FBV_BITS_PER_ENTRY);

  if (single_indirect_blocknum == 0x00000000) 
  {
    dbprintf ('d', "Leaving check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated: single_indirect_blocknum has the default value and is NOT allocated\n");
    return DFS_FAIL;
  }
  else if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000) 
  {
    dbprintf ('d', "Leaving check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated: single_indirect_blocknum NOT allocated\n");
    return DFS_FAIL;
  }
  else 
  {
    dbprintf ('d', "Leaving check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated: single_indirect_blocknum IS allocated\n");
    return DFS_SUCCESS;
  }
}

int check_block_in_double_indirect_addressed_table_to_be_allocated (uint32 handle, uint32 virtual_blocknum)
{
  uint32 dfs_blocknum;
  int bit_num;
  int fbv_index;
  
  dbprintf ('d', "Entering check_block_in_double_indirect_addressed_table_to_be_allocated\n");

  dfs_blocknum = get_dfs_blocknum_within_double_indirect_addressed_blocks (inodes[handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
  if (dfs_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside check_block_in_double_indirect_addressed_table_to_be_allocated: get_dfs_blocknum_within_double_indirect_addressed_blocks FAILED. dfs_blocknum = %d\n", dfs_blocknum);
    return DFS_FAIL;
  }
  
// Check the dfs_blocknum to be allocated
  fbv_index = (dfs_blocknum / DFS_FBV_BITS_PER_ENTRY);
  bit_num = (dfs_blocknum % DFS_FBV_BITS_PER_ENTRY);

  if (dfs_blocknum == 0x00000000) 
  {
    dbprintf ('d', "Leaving check_block_in_double_indirect_addressed_table_to_be_allocated: dfs_blocknum has the default value and is NOT allocated\n");
    return DFS_FAIL;
  }
  else if ((read_fbv(fbv_index) & (1 << bit_num)) == 0x00000000) 
  {
    dbprintf ('d', "Leaving check_block_in_double_indirect_addressed_table_to_be_allocated: dfs_blocknum NOT allocated\n");
    return DFS_FAIL;
  }
  else 
  {
    dbprintf ('d', "Leaving check_block_in_double_indirect_addressed_table_to_be_allocated: dfs_blocknum IS allocated\n");
    return DFS_SUCCESS;
  }
}

int DfsInodeWriteBytes_in_one_block (uint32 handle, void *mem, int start_byte, int num_bytes) 
{
  int i;
  uint32 virtual_blocknum;
  int virtual_byteoffset;
  uint32 dfs_blocknum;
  int cache_result;
  dfs_block dfsb;
  int end_byte;

  dbprintf ('d', "Entering DfsInodeWriteBytes_in_one_block\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: filesystem is not opened\n");
    return DFS_FAIL;
  }

  virtual_blocknum = (start_byte / sb.dfs_blocksize);
  virtual_byteoffset = (start_byte % sb.dfs_blocksize);
  
// 1) Checking if the virtual block falls in direct addressing table. Returns the content of array
  if (virtual_blocknum < DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS)
  {
    if((inodes[handle].direct_addressed_blocks[virtual_blocknum]) == 0x00000000)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else
    {
      dfs_blocknum = inodes[handle].direct_addressed_blocks[virtual_blocknum];
    }
  }
// 2) Checking if the virtual block falls in single-indirect addressing table. Return the value of dfs blocknum from the single-indirect address dfs blocknum
  else if ((virtual_blocknum >= DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS) && (virtual_blocknum < (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)))
  {
    if(inodes[handle].single_indirect_addressed_blocks_dfs_block_num == 0x00000000)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else if(check_block_in_single_indirect_addressed_table_to_be_allocated(handle, virtual_blocknum) == DFS_FAIL)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else
    {
      dfs_blocknum = get_dfs_blocknum_within_single_indirect_addressed_blocks(inodes[handle].single_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: get_dfs_blocknum_within_single_indirect_addressed_blocks Failed\n");
        return DFS_FAIL;
      }
    }
  }
// 3) Checking if the virtual block falls in double-indirect addressing table. Get the single-indirect address blocknum. Return the value of dfs blocknum from the single-indirect address dfs blocknum
  else if ((virtual_blocknum >= (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)) && (virtual_blocknum < DFS_INODE_MAX_VIRTUAL_BLOCKNUM))
  {
    if(inodes[handle].double_indirect_addressed_blocks_dfs_block_num == 0x00000000)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else if(check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated(handle, virtual_blocknum) == DFS_FAIL)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else if(check_block_in_double_indirect_addressed_table_to_be_allocated(handle, virtual_blocknum) == DFS_FAIL)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else
    {
      dfs_blocknum = get_dfs_blocknum_within_double_indirect_addressed_blocks(inodes[handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: get_dfs_blocknum_within_double_indirect_addressed_blocks Failed\n");
        return DFS_FAIL;
      }
    }
  }
// 4) Return with ERROR, if virtual block exceeds the max limit
  else
  {
    printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: Virtual blocknum exceeded max limit. virtual_blocknum = %d\n", virtual_blocknum);
    return DFS_FAIL;
  }

  end_byte = start_byte + num_bytes - 1;
  dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: start_byte = %d, num_bytes = %d, end_byte = %d\n", start_byte, num_bytes, end_byte);
  dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: OLD inodes[handle].current_file_size = %d\n", inodes[handle].current_file_size);
  if (end_byte > inodes[handle].current_file_size)
  {
    inodes[handle].current_file_size = inodes[handle].current_file_size + (end_byte - inodes[handle].current_file_size);
  }
  else
  {
    inodes[handle].current_file_size = inodes[handle].current_file_size;
  }
  dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: NEW inodes[handle].current_file_size = %d\n", inodes[handle].current_file_size);

/*  
  for (i=0; i<num_bytes; i++)
  {
    dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: *(char *)(mem + %d) = %d\n", i, *(char *)(mem + i));
  }
  dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: num_bytes = %d\n", num_bytes);
*/
  if (virtual_byteoffset == 0)
  {
    dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: Writing without Reading the dfs_blocknum = %d\n", dfs_blocknum);
    //bzero((char *)&dfsb, sb.dfs_blocksize);
    bzero(dfsb.data, sb.dfs_blocksize);
    bcopy ((char *)mem, (dfsb.data), num_bytes);
/*
    for (i=0; i<sb.dfs_blocksize; i++)
    {
      dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: dfsb.data[%d] = %d\n", i, dfsb.data[i]);
    }
*/
    cache_result = DfsCacheWrite (dfs_blocknum, &dfsb);
    if (cache_result == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsCacheWrite FAILED.\n");
      return DFS_FAIL;
    }
  }
  else
  {
    dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: Writing by first Reading and then upating the dfs_blocknum = %d\n", dfs_blocknum);

    cache_result = DfsCacheRead (dfs_blocknum, &dfsb);
    if (cache_result == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsCacheRead FAILED.\n");
      return DFS_FAIL;
    }
    
    bcopy ((char *)mem, (dfsb.data + virtual_byteoffset), num_bytes);
/*
    for (i=0; i<sb.dfs_blocksize; i++)
    {
      dbprintf ('d', "Inside DfsInodeWriteBytes_in_one_block: dfsb.data[%d] = %d\n", i, dfsb.data[i]);
    }
*/
    cache_result = DfsCacheWrite (dfs_blocknum, &dfsb);
    if (cache_result == DFS_FAIL)
    {
      printf("ERROR: Inside DfsInodeWriteBytes_in_one_block: DfsCacheWrite FAILED.\n");
      return DFS_FAIL;
    }
  }
  
  dbprintf ('d', "Leaving DfsInodeWriteBytes_in_one_block\n");
  return num_bytes;
}

int DfsInodeWriteBytes (uint32 handle, void *mem, int start_byte, int num_bytes)
{
  int num_bytes_returned;
  int end_byte;
  int start_blocknum;
  int end_blocknum;
  int current_byte;
  int current_blocknum;
  int num_bytes_in_block;
  int mem_ptr_offset;
  int num_bytes_written;

  dbprintf ('d', "Entering DfsInodeWriteBytes\n");

  end_byte = start_byte + num_bytes - 1;

  if(end_byte >= DFS_MAX_FILESYSTEM_SIZE)
  {
    end_byte = DFS_MAX_FILESYSTEM_SIZE - 1;
  }
  
  start_blocknum = (start_byte / sb.dfs_blocksize);
  end_blocknum = (end_byte / sb.dfs_blocksize);
  
  mem_ptr_offset = 0;
  num_bytes_written = 0;

  dbprintf ('d', "start_byte = %d, num_bytes = %d\n", start_byte, num_bytes);
  dbprintf ('d', "start_byte = %d, start_blocknum = %d, end_byte = %d, end_blocknum = %d\n", start_byte, start_blocknum, end_byte, end_blocknum);

  for (current_byte=start_byte, current_blocknum=start_blocknum; current_blocknum<=end_blocknum; current_blocknum++, current_byte=current_blocknum*sb.dfs_blocksize)
  { 
    if (start_blocknum == end_blocknum)
    {
      num_bytes_in_block = num_bytes;
    }
    else if (current_blocknum == start_blocknum)
    {
      num_bytes_in_block = (((start_blocknum + 1) * sb.dfs_blocksize) - start_byte);
    }
    else if (current_blocknum == end_blocknum)
    {
      num_bytes_in_block = (end_byte - (end_blocknum * sb.dfs_blocksize) + 1);
    }
    else
    {
      num_bytes_in_block = sb.dfs_blocksize;
    }

    dbprintf ('d', "current_blocknum = %d, current_byte = %d, mem_ptr_offset = %d, num_bytes_in_block = %d\n", current_blocknum, current_byte, mem_ptr_offset, num_bytes_in_block);  

    num_bytes_returned = DfsInodeWriteBytes_in_one_block (handle, (char *)(mem + mem_ptr_offset), current_byte, num_bytes_in_block);
    if (num_bytes_returned != num_bytes_in_block)
    {
      printf("ERROR: Inside DfsInodeWriteBytes: DfsInodeWriteBytes_in_one_block FAILED. num_bytes_returned = %d\n", num_bytes_returned);
      return DFS_FAIL;
    }
    
    mem_ptr_offset = mem_ptr_offset + num_bytes_in_block;
    num_bytes_written = num_bytes_written + num_bytes_returned;
  }

  dbprintf ('d', "Leaving DfsInodeWriteBytes\n");
  return num_bytes_written;
}

int DfsInodeWriteBytesUncached_in_one_block (uint32 handle, void *mem, int start_byte, int num_bytes) 
{
  int i;
  uint32 virtual_blocknum;
  int virtual_byteoffset;
  uint32 dfs_blocknum;
  int dfs_blocksize_returned;
  dfs_block dfsb;
  int end_byte;

  dbprintf ('d', "Entering DfsInodeWriteBytesUncached_in_one_block\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: filesystem is not opened\n");
    return DFS_FAIL;
  }

  virtual_blocknum = (start_byte / sb.dfs_blocksize);
  virtual_byteoffset = (start_byte % sb.dfs_blocksize);
  
// 1) Checking if the virtual block falls in direct addressing table. Returns the content of array
  if (virtual_blocknum < DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS)
  {
    if((inodes[handle].direct_addressed_blocks[virtual_blocknum]) == 0x00000000)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else
    {
      dfs_blocknum = inodes[handle].direct_addressed_blocks[virtual_blocknum];
    }
  }
// 2) Checking if the virtual block falls in single-indirect addressing table. Return the value of dfs blocknum from the single-indirect address dfs blocknum
  else if ((virtual_blocknum >= DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS) && (virtual_blocknum < (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)))
  {
    if(inodes[handle].single_indirect_addressed_blocks_dfs_block_num == 0x00000000)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else if(check_block_in_single_indirect_addressed_table_to_be_allocated(handle, virtual_blocknum) == DFS_FAIL)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else
    {
      dfs_blocknum = get_dfs_blocknum_within_single_indirect_addressed_blocks(inodes[handle].single_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: get_dfs_blocknum_within_single_indirect_addressed_blocks Failed\n");
        return DFS_FAIL;
      }
    }
  }
// 3) Checking if the virtual block falls in double-indirect addressing table. Get the single-indirect address blocknum. Return the value of dfs blocknum from the single-indirect address dfs blocknum
  else if ((virtual_blocknum >= (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)) && (virtual_blocknum < DFS_INODE_MAX_VIRTUAL_BLOCKNUM))
  {
    if(inodes[handle].double_indirect_addressed_blocks_dfs_block_num == 0x00000000)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else if(check_single_indirect_addressed_table_in_double_indirect_addressed_table_to_be_allocated(handle, virtual_blocknum) == DFS_FAIL)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else if(check_block_in_double_indirect_addressed_table_to_be_allocated(handle, virtual_blocknum) == DFS_FAIL)
    {
      dfs_blocknum = DfsInodeAllocateVirtualBlock (handle, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsInodeAllocateVirtualBlock Failed\n");
        return DFS_FAIL;          
      }
    }
    else
    {
      dfs_blocknum = get_dfs_blocknum_within_double_indirect_addressed_blocks(inodes[handle].double_indirect_addressed_blocks_dfs_block_num, virtual_blocknum);
      if (dfs_blocknum == DFS_FAIL)
      {
        printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: get_dfs_blocknum_within_double_indirect_addressed_blocks Failed\n");
        return DFS_FAIL;
      }
    }
  }
// 4) Return with ERROR, if virtual block exceeds the max limit
  else
  {
    printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: Virtual blocknum exceeded max limit. virtual_blocknum = %d\n", virtual_blocknum);
    return DFS_FAIL;
  }

  end_byte = start_byte + num_bytes - 1;
  dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: start_byte = %d, num_bytes = %d, end_byte = %d\n", start_byte, num_bytes, end_byte);
  dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: OLD inodes[handle].current_file_size = %d\n", inodes[handle].current_file_size);
  if (end_byte > inodes[handle].current_file_size)
  {
    inodes[handle].current_file_size = inodes[handle].current_file_size + (end_byte - inodes[handle].current_file_size);
  }
  else
  {
    inodes[handle].current_file_size = inodes[handle].current_file_size;
  }
  dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: NEW inodes[handle].current_file_size = %d\n", inodes[handle].current_file_size);

/*  
  for (i=0; i<num_bytes; i++)
  {
    dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: *(char *)(mem + %d) = %d\n", i, *(char *)(mem + i));
  }
  dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: num_bytes = %d\n", num_bytes);
*/
  if (virtual_byteoffset == 0)
  {
    dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: Writing without Reading the dfs_blocknum = %d\n", dfs_blocknum);
    //bzero((char *)&dfsb, sb.dfs_blocksize);
    bzero(dfsb.data, sb.dfs_blocksize);
    bcopy ((char *)mem, (dfsb.data), num_bytes);
/*
    for (i=0; i<sb.dfs_blocksize; i++)
    {
      dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: dfsb.data[%d] = %d\n", i, dfsb.data[i]);
    }
*/
    dfs_blocksize_returned = DfsWriteBlock (dfs_blocknum, &dfsb);
    if (dfs_blocksize_returned != sb.dfs_blocksize)
    {
      printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsWriteBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
      return DFS_FAIL;
    }
  }
  else
  {
    dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: Writing by first Reading and then upating the dfs_blocknum = %d\n", dfs_blocknum);

    dfs_blocksize_returned = DfsReadBlock (dfs_blocknum, &dfsb);
    if (dfs_blocksize_returned != sb.dfs_blocksize)
    {
      printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
      return DFS_FAIL;
    }
    
    bcopy ((char *)mem, (dfsb.data + virtual_byteoffset), num_bytes);
/*
    for (i=0; i<sb.dfs_blocksize; i++)
    {
      dbprintf ('d', "Inside DfsInodeWriteBytesUncached_in_one_block: dfsb.data[%d] = %d\n", i, dfsb.data[i]);
    }
*/
    dfs_blocksize_returned = DfsWriteBlock (dfs_blocknum, &dfsb);
    if (dfs_blocksize_returned != sb.dfs_blocksize)
    {
      printf("ERROR: Inside DfsInodeWriteBytesUncached_in_one_block: DfsWriteBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
      return DFS_FAIL;
    }
  }
  
  dbprintf ('d', "Leaving DfsInodeWriteBytesUncached_in_one_block\n");
  return num_bytes;
}

int DfsInodeWriteBytesUncached (uint32 handle, void *mem, int start_byte, int num_bytes)
{
  int num_bytes_returned;
  int end_byte;
  int start_blocknum;
  int end_blocknum;
  int current_byte;
  int current_blocknum;
  int num_bytes_in_block;
  int mem_ptr_offset;
  int num_bytes_written;

  dbprintf ('d', "Entering DfsInodeWriteBytesUncached\n");

  end_byte = start_byte + num_bytes - 1;

  if(end_byte >= DFS_MAX_FILESYSTEM_SIZE)
  {
    end_byte = DFS_MAX_FILESYSTEM_SIZE - 1;
  }
  
  start_blocknum = (start_byte / sb.dfs_blocksize);
  end_blocknum = (end_byte / sb.dfs_blocksize);
  
  mem_ptr_offset = 0;
  num_bytes_written = 0;

  dbprintf ('d', "start_byte = %d, num_bytes = %d\n", start_byte, num_bytes);
  dbprintf ('d', "start_byte = %d, start_blocknum = %d, end_byte = %d, end_blocknum = %d\n", start_byte, start_blocknum, end_byte, end_blocknum);

  for (current_byte=start_byte, current_blocknum=start_blocknum; current_blocknum<=end_blocknum; current_blocknum++, current_byte=current_blocknum*sb.dfs_blocksize)
  { 
    if (start_blocknum == end_blocknum)
    {
      num_bytes_in_block = num_bytes;
    }
    else if (current_blocknum == start_blocknum)
    {
      num_bytes_in_block = (((start_blocknum + 1) * sb.dfs_blocksize) - start_byte);
    }
    else if (current_blocknum == end_blocknum)
    {
      num_bytes_in_block = (end_byte - (end_blocknum * sb.dfs_blocksize) + 1);
    }
    else
    {
      num_bytes_in_block = sb.dfs_blocksize;
    }

    dbprintf ('d', "current_blocknum = %d, current_byte = %d, mem_ptr_offset = %d, num_bytes_in_block = %d\n", current_blocknum, current_byte, mem_ptr_offset, num_bytes_in_block);  

    num_bytes_returned = DfsInodeWriteBytesUncached_in_one_block (handle, (char *)(mem + mem_ptr_offset), current_byte, num_bytes_in_block);
    if (num_bytes_returned != num_bytes_in_block)
    {
      printf("ERROR: Inside DfsInodeWriteBytesUncached: DfsInodeWriteBytesUncached_in_one_block FAILED. num_bytes_returned = %d\n", num_bytes_returned);
      return DFS_FAIL;
    }
    
    mem_ptr_offset = mem_ptr_offset + num_bytes_in_block;
    num_bytes_written = num_bytes_written + num_bytes_returned;
  }

  dbprintf ('d', "Leaving DfsInodeWriteBytesUncached\n");
  return num_bytes_written;
}

//-----------------------------------------------------------------
// DfsInodeReadBytes reads num_bytes from the file represented by 
// the inode handle, starting at virtual byte start_byte, copying 
// the data to the address pointed to by mem. Return DFS_FAIL on 
// failure, and the number of bytes read on success.
//-----------------------------------------------------------------

int DfsInodeReadBytes_in_one_block(uint32 handle, void *mem, int start_byte, int num_bytes) 
{
  uint32 virtual_blocknum;
  int virtual_byteoffset;
  uint32 dfs_blocknum;
  int cache_result;
  dfs_block dfsb;

  dbprintf ('d', "Entering DfsInodeReadBytes_in_one_block\n");

  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeReadBytes_in_one_block: filesystem is not opened\n");
    return DFS_FAIL;
  }

  virtual_blocknum = (start_byte / sb.dfs_blocksize);
  virtual_byteoffset = (start_byte % sb.dfs_blocksize);
  
  dfs_blocknum = DfsInodeTranslateVirtualToFilesys(handle, virtual_blocknum);
  if (dfs_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside DfsInodeReadBytes_in_one_block: DfsInodeTranslateVirtualToFilesys Failed\n");
    return DFS_FAIL;
  }
  
  dbprintf ('d', "Inside DfsInodeReadBytes_in_one_block: Reading from dfs_blocknum = %d\n", dfs_blocknum);
  cache_result = DfsCacheRead (dfs_blocknum, &dfsb);
  if (cache_result == DFS_FAIL)
  {
    printf("ERROR: Inside DfsInodeReadBytes_in_one_block: DfsCacheRead FAILED.\n");
    return DFS_FAIL;
  }
  
  bcopy ((dfsb.data + virtual_byteoffset), (char *)mem, num_bytes);
  
  dbprintf ('d', "Leaving DfsInodeReadBytes_in_one_block\n");
  return num_bytes;
}

int DfsInodeReadBytes (uint32 handle, void *mem, int start_byte, int num_bytes)
{
  int num_bytes_returned;
  int end_byte;
  int start_blocknum;
  int end_blocknum;
  int current_byte;
  int current_blocknum;
  int num_bytes_in_block;
  int mem_ptr_offset;
  int num_bytes_read;

  dbprintf ('d', "Entering DfsInodeReadBytes\n");

  end_byte = start_byte + num_bytes - 1;

  if(end_byte >= DFS_MAX_FILESYSTEM_SIZE)
  {
    end_byte = DFS_MAX_FILESYSTEM_SIZE - 1;
  }
  
  start_blocknum = (start_byte / sb.dfs_blocksize);
  end_blocknum = (end_byte / sb.dfs_blocksize);
  
  mem_ptr_offset = 0;
  num_bytes_read = 0;

  for (current_byte=start_byte, current_blocknum=start_blocknum; current_blocknum<=end_blocknum; current_blocknum++, current_byte=current_blocknum*sb.dfs_blocksize)
  { 
    if (start_blocknum == end_blocknum)
    {
      num_bytes_in_block = num_bytes;
    }
    else if (current_blocknum == start_blocknum)
    {
      num_bytes_in_block = (((start_blocknum + 1) * sb.dfs_blocksize) - start_byte);
    }
    else if (current_blocknum == end_blocknum)
    {
      num_bytes_in_block = (end_byte - (end_blocknum * sb.dfs_blocksize) + 1);
    }
    else
    {
      num_bytes_in_block = sb.dfs_blocksize;
    }

    num_bytes_returned = DfsInodeReadBytes_in_one_block (handle, (char *)(mem + mem_ptr_offset), current_byte, num_bytes_in_block);
    if (num_bytes_returned != num_bytes_in_block)
    {
      printf("ERROR: Inside DfsInodeReadBytes: DfsInodeReadBytes_in_one_block FAILED. num_bytes_returned = %d\n", num_bytes_returned);
      return DFS_FAIL;
    }
    
    mem_ptr_offset = mem_ptr_offset + num_bytes_in_block;
    num_bytes_read = num_bytes_read + num_bytes_returned;
  }

  dbprintf ('d', "Leaving DfsInodeReadBytes\n");
  return num_bytes_read;
}


int DfsInodeReadBytesUncached_in_one_block(uint32 handle, void *mem, int start_byte, int num_bytes) 
{
  uint32 virtual_blocknum;
  int virtual_byteoffset;
  uint32 dfs_blocknum;
  int dfs_blocksize_returned;
  dfs_block dfsb;

  dbprintf ('d', "Entering DfsInodeReadBytesUncached_in_one_block\n");

  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeReadBytesUncached_in_one_block: filesystem is not opened\n");
    return DFS_FAIL;
  }

  virtual_blocknum = (start_byte / sb.dfs_blocksize);
  virtual_byteoffset = (start_byte % sb.dfs_blocksize);
  
  dfs_blocknum = DfsInodeTranslateVirtualToFilesys(handle, virtual_blocknum);
  if (dfs_blocknum == DFS_FAIL)
  {
    printf("ERROR: Inside DfsInodeReadBytesUncached_in_one_block: DfsInodeTranslateVirtualToFilesys Failed\n");
    return DFS_FAIL;
  }
  
  dbprintf ('d', "Inside DfsInodeReadBytesUncached_in_one_block: Reading from dfs_blocknum = %d\n", dfs_blocknum);
  dfs_blocksize_returned = DfsReadBlock (dfs_blocknum, &dfsb);
  if (dfs_blocksize_returned != sb.dfs_blocksize)
  {
    printf("ERROR: Inside DfsInodeReadBytesUncached_in_one_block: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
    return DFS_FAIL;
  }
  
  bcopy ((dfsb.data + virtual_byteoffset), (char *)mem, num_bytes);
  
  dbprintf ('d', "Leaving DfsInodeReadBytesUncached_in_one_block\n");
  return num_bytes;
}

int DfsInodeReadBytesUncachedUncached (uint32 handle, void *mem, int start_byte, int num_bytes)
{
  int num_bytes_returned;
  int end_byte;
  int start_blocknum;
  int end_blocknum;
  int current_byte;
  int current_blocknum;
  int num_bytes_in_block;
  int mem_ptr_offset;
  int num_bytes_read;

  dbprintf ('d', "Entering DfsInodeReadBytesUncached\n");

  end_byte = start_byte + num_bytes - 1;

  if(end_byte >= DFS_MAX_FILESYSTEM_SIZE)
  {
    end_byte = DFS_MAX_FILESYSTEM_SIZE - 1;
  }
  
  start_blocknum = (start_byte / sb.dfs_blocksize);
  end_blocknum = (end_byte / sb.dfs_blocksize);
  
  mem_ptr_offset = 0;
  num_bytes_read = 0;

  for (current_byte=start_byte, current_blocknum=start_blocknum; current_blocknum<=end_blocknum; current_blocknum++, current_byte=current_blocknum*sb.dfs_blocksize)
  { 
    if (start_blocknum == end_blocknum)
    {
      num_bytes_in_block = num_bytes;
    }
    else if (current_blocknum == start_blocknum)
    {
      num_bytes_in_block = (((start_blocknum + 1) * sb.dfs_blocksize) - start_byte);
    }
    else if (current_blocknum == end_blocknum)
    {
      num_bytes_in_block = (end_byte - (end_blocknum * sb.dfs_blocksize) + 1);
    }
    else
    {
      num_bytes_in_block = sb.dfs_blocksize;
    }

    num_bytes_returned = DfsInodeReadBytesUncached_in_one_block (handle, (char *)(mem + mem_ptr_offset), current_byte, num_bytes_in_block);
    if (num_bytes_returned != num_bytes_in_block)
    {
      printf("ERROR: Inside DfsInodeReadBytesUncached: DfsInodeReadBytesUncached_in_one_block FAILED. num_bytes_returned = %d\n", num_bytes_returned);
      return DFS_FAIL;
    }
    
    mem_ptr_offset = mem_ptr_offset + num_bytes_in_block;
    num_bytes_read = num_bytes_read + num_bytes_returned;
  }

  dbprintf ('d', "Leaving DfsInodeReadBytesUncached\n");
  return num_bytes_read;
}


//-----------------------------------------------------------------
// DfsInodeFilesize simply returns the size of an inode's file. 
// This is defined as the maximum virtual byte number that has 
// been written to the inode thus far. Return DFS_FAIL on failure.
//-----------------------------------------------------------------

uint32 DfsInodeFilesize(uint32 handle) 
{
  dbprintf ('d', "Entering DfsInodeFilesize\n");

  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsInodeFilesize: filesystem is not opened\n");
    return DFS_FAIL;
  }

  dbprintf ('d', "Leaving DfsInodeFilesize\n");
  return inodes[handle].current_file_size;
}



////////////////////////////////////////////////////////////////////////////////
// Buffer Cache based functions
////////////////////////////////////////////////////////////////////////////////

int read_buffer_cache_inuse (int buffer_cache_handle)
{
  int inuse;
  int intrs;

  //dbprintf ('b', "Entering read_buffer_cache_inuse\n");

  intrs = DisableIntrs ();
  inuse = buffer_cache_tag[buffer_cache_handle].inuse;
  RestoreIntrs (intrs);

  //dbprintf ('b', "Leaving read_buffer_cache_inuse\n");

  return inuse;
}

int read_buffer_cache_dirty (int buffer_cache_handle)
{
  int dirty;
  int intrs;

  //dbprintf ('b', "Entering read_buffer_cache_dirty\n");

  intrs = DisableIntrs ();
  dirty = buffer_cache_tag[buffer_cache_handle].dirty;
  RestoreIntrs (intrs);

  //dbprintf ('b', "Leaving read_buffer_cache_dirty\n");

  return dirty;
}

int read_buffer_cache_dfs_blocknum (int buffer_cache_handle)
{
  int dfs_blocknum;
  int intrs;

  //dbprintf ('b', "Entering read_buffer_cache_dfs_blocknum\n");

  intrs = DisableIntrs ();
  dfs_blocknum = buffer_cache_tag[buffer_cache_handle].dfs_blocknum;
  RestoreIntrs (intrs);

  //dbprintf ('b', "Leaving read_buffer_cache_dfs_blocknum\n");

  return dfs_blocknum;
}

int read_buffer_cache_num_file_op_count (int buffer_cache_handle)
{
  int num_file_op_count;
  int intrs;

  //dbprintf ('b', "Entering read_buffer_cache_num_file_op_count\n");

  intrs = DisableIntrs ();
  num_file_op_count = buffer_cache_tag[buffer_cache_handle].num_file_op_count;
  RestoreIntrs (intrs);

  //dbprintf ('b', "Leaving read_buffer_cache_num_file_op_count\n");

  return num_file_op_count;
}

int DfsCacheHit(int blocknum)
{
  int i;
  int hit;
  int buffer_cache_handle;

  //dbprintf ('b', "Entering DfsCacheHit\n");

  hit = 0;
  for (i=0; i<DFS_BUFFER_CACHE_NUM_SLOTS; i++)
  {
    if (read_buffer_cache_dfs_blocknum(i) == blocknum)
    {
      if (read_buffer_cache_inuse(i) == 1)
      {
        hit = 1;
        buffer_cache_handle = i;
        break;
      }
    }
  }

  if (hit == 1)
  {
    dbprintf ('b', "Leaving DfsCacheHit: Buffer Cache HIT\n");
    return buffer_cache_handle;
  }
  else
  {
    dbprintf ('b', "Leaving DfsCacheHit: Buffer Cache MISS\n");
    return DFS_FAIL;
  }
}

int check_empty_slot_in_buffer_cache ()
{
  int i;
  int hit;
  int buffer_cache_handle;

  //dbprintf ('b', "Entering check_empty_slot_in_buffer_cache\n");
  
  hit = 0;
  for (i=0; i<DFS_BUFFER_CACHE_NUM_SLOTS; i++)
  {
    if (read_buffer_cache_inuse(i) == 0)
    {
      hit = 1;
      buffer_cache_handle = i;
      break;
    }
  }

  if (hit == 1)
  {
    dbprintf ('b', "Leaving check_empty_slot_in_buffer_cache: Empty slot FOUND at buffer_cache_handle = %d\n", buffer_cache_handle);
    return buffer_cache_handle;
  }
  else
  {
    dbprintf ('b', "Leaving check_empty_slot_in_buffer_cache: Empty slot NOT FOUND \n");
    return DFS_FAIL;
  }
}

int replace_lfu_slot ()
{
  int i;
  int smallest;
  int lfu_buffer_cache_handle;
  int dfs_blocksize_returned;
  int intrs;
  
  //dbprintf ('b', "Entering replace_lfu_slot\n");

  lfu_buffer_cache_handle = 0;
  smallest = read_buffer_cache_num_file_op_count(lfu_buffer_cache_handle);
  for (i=0; i<DFS_BUFFER_CACHE_NUM_SLOTS; i++)
  {
    if (read_buffer_cache_num_file_op_count(i) < smallest)
    {
      lfu_buffer_cache_handle = i;
      smallest = read_buffer_cache_num_file_op_count(lfu_buffer_cache_handle);
    }
  }

  if (read_buffer_cache_dirty(lfu_buffer_cache_handle) == 1)
  {
    dfs_blocksize_returned = DfsWriteBlock(read_buffer_cache_dfs_blocknum(lfu_buffer_cache_handle), &buffer_cache_data[lfu_buffer_cache_handle]);
    if (dfs_blocksize_returned != sb.dfs_blocksize)
    {
      printf("ERROR: Inside replace_lfu_slot: DfsWriteBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
      return DFS_FAIL;
    }

    dbprintf ('b', "Inside replace_lfu_slot: Written back to disk the dfs_blocknum = %d, at lfu_buffer_cache_handle = %d\n", read_buffer_cache_dfs_blocknum(lfu_buffer_cache_handle), lfu_buffer_cache_handle);
    intrs = DisableIntrs ();
    buffer_cache_tag[lfu_buffer_cache_handle].dirty = 0;    
    RestoreIntrs (intrs);
  }

  intrs = DisableIntrs ();
  buffer_cache_tag[lfu_buffer_cache_handle].dfs_blocknum = 0x00000000;
  buffer_cache_tag[lfu_buffer_cache_handle].num_file_op_count = 0;
  buffer_cache_tag[lfu_buffer_cache_handle].inuse = 0;
  RestoreIntrs (intrs);
 
  dbprintf ('b', "Leaving replace_lfu_slot: Replaced dfs_blocknum = %d, at lfu_buffer_cache_handle = %d\n", read_buffer_cache_dfs_blocknum(lfu_buffer_cache_handle), lfu_buffer_cache_handle);
  return lfu_buffer_cache_handle;
}

int DfsCacheAllocateSlot(int blocknum)
{
  int buffer_cache_handle;
  int intrs;

  //dbprintf ('b', "Entering DfsCacheAllocateSlot\n");

  buffer_cache_handle = check_empty_slot_in_buffer_cache ();
  if (buffer_cache_handle == DFS_FAIL)
  {
    buffer_cache_handle = replace_lfu_slot();
    if (buffer_cache_handle == DFS_FAIL)
    {
      printf("ERROR: Inside DfsCacheAllocateSlot: check_empty_slot_in_buffer_cache FAILED.\n");
      return DFS_FAIL;
    }
  
    intrs = DisableIntrs ();
    buffer_cache_tag[buffer_cache_handle].dfs_blocknum = blocknum;
    buffer_cache_tag[buffer_cache_handle].num_file_op_count = 0;
    buffer_cache_tag[buffer_cache_handle].dirty = 0;
    buffer_cache_tag[buffer_cache_handle].inuse = 1;
    RestoreIntrs (intrs);

    dbprintf ('b', "Leaving DfsCacheAllocateSlot: Allocated slot after replacing the buffer_cache_handle = %d\n", buffer_cache_handle);
    return buffer_cache_handle;
  }
  else
  {
    intrs = DisableIntrs ();
    buffer_cache_tag[buffer_cache_handle].dfs_blocknum = blocknum;
    buffer_cache_tag[buffer_cache_handle].num_file_op_count = 0;
    buffer_cache_tag[buffer_cache_handle].dirty = 0;
    buffer_cache_tag[buffer_cache_handle].inuse = 1;
    RestoreIntrs (intrs);

    dbprintf ('b', "Leaving DfsCacheAllocateSlot: Allocated buffer_cache_handle = %d\n", buffer_cache_handle);
    return buffer_cache_handle;
  }
}

int DfsCacheRead(int blocknum, dfs_block *cache_block)
{
  int buffer_cache_handle;
  dfs_block dfsb;
  int dfs_blocksize_returned;
  int intrs;
  int old_num_file_op_count;

  //dbprintf ('b', "Entering DfsCacheRead\n");

  buffer_cache_handle = DfsCacheHit(blocknum);
  if (buffer_cache_handle == DFS_FAIL)
  {
    dbprintf ('b', "Inside DfsCacheRead: Reading block from Disk into the Cache. blocknum = %d\n", blocknum);

    buffer_cache_handle = DfsCacheAllocateSlot(blocknum);
    if (buffer_cache_handle == DFS_FAIL)
    {
      printf("ERROR: Inside DfsCacheRead: DfsCacheAllocateSlot FAILED\n");
      return DFS_FAIL;
    }

    dfs_blocksize_returned = DfsReadBlock (blocknum, &dfsb);
    if (dfs_blocksize_returned != sb.dfs_blocksize)
    {
      printf("ERROR: Inside DfsCacheRead: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
      return DFS_FAIL;
    }
    intrs = DisableIntrs ();
    bcopy (dfsb.data, buffer_cache_data[buffer_cache_handle].data, sb.dfs_blocksize);
    RestoreIntrs (intrs);
  }

  old_num_file_op_count = read_buffer_cache_num_file_op_count(buffer_cache_handle);

  intrs = DisableIntrs ();
  bcopy(buffer_cache_data[buffer_cache_handle].data, cache_block->data, sb.dfs_blocksize);
  buffer_cache_tag[buffer_cache_handle].num_file_op_count = old_num_file_op_count + 1;
  RestoreIntrs (intrs);

  //dbprintf ('b', "Leaving DfsCacheRead\n");
  return DFS_SUCCESS;
}

int DfsCacheWrite(int blocknum, dfs_block *cache_block)
{
  int buffer_cache_handle;
  dfs_block dfsb;
  int dfs_blocksize_returned;
  int old_num_file_op_count;
  int intrs;

  //dbprintf ('b', "Entering DfsCacheWrite\n");

  buffer_cache_handle = DfsCacheHit(blocknum);
  if (buffer_cache_handle == DFS_FAIL)
  {
    dbprintf ('b', "Inside DfsCacheWrite: Reading block from Disk into the Cache. blocknum = %d\n", blocknum);

    buffer_cache_handle = DfsCacheAllocateSlot(blocknum);
    if (buffer_cache_handle == DFS_FAIL)
    {
      printf("ERROR: Inside DfsCacheWrite: DfsCacheAllocateSlot FAILED\n");
      return DFS_FAIL;
    }

    dfs_blocksize_returned = DfsReadBlock (blocknum, &dfsb);
    if (dfs_blocksize_returned != sb.dfs_blocksize)
    {
      printf("ERROR: Inside DfsCacheWrite: DfsReadBlock FAILED. dfs_blocksize_returned = %d\n", dfs_blocksize_returned);
      return DFS_FAIL;
    }
    intrs = DisableIntrs ();
    bcopy (dfsb.data, buffer_cache_data[buffer_cache_handle].data, sb.dfs_blocksize);
    RestoreIntrs (intrs);
  }

  old_num_file_op_count = read_buffer_cache_num_file_op_count(buffer_cache_handle);

  intrs = DisableIntrs ();
  bcopy(cache_block->data, buffer_cache_data[buffer_cache_handle].data, sb.dfs_blocksize);
  buffer_cache_tag[buffer_cache_handle].num_file_op_count = old_num_file_op_count + 1;
  buffer_cache_tag[buffer_cache_handle].dirty = 1;
  RestoreIntrs (intrs);

  //dbprintf ('b', "Leaving DfsCacheWrite\n");
  return DFS_SUCCESS;
}

int DfsWriteBlockForCacheFlush (uint32 blocknum, dfs_block *b)
{
  int i, j;
  disk_block db;
  uint32 disk_blocknum;
  int disk_block_size_returned;

  //dbprintf ('b', "Entering DfsWriteBlockForCacheFlush\n");

// 1) Checking filesystem to be opened
  if (read_sb_valid() == 0)
  {
    printf("ERROR: Inside DfsWriteBlockForCacheFlush: filesystem is not opened\n");
    return DFS_FAIL;
  }

// 2) Writing the (set of) block(s) to the disk
  //dbprintf ('b', "Inside DfsWriteBlockForCacheFlush: blocknum = %d\n", blocknum);
  disk_blocknum = (blocknum * (sb.dfs_blocksize / DISK_BLOCKSIZE));
  for (i=0; i<(sb.dfs_blocksize / DISK_BLOCKSIZE); i++, disk_blocknum++)
  {
    //dbprintf ('b', "Inside DfsWriteBlockForCacheFlush: i = %d\n", i);
    dbprintf ('b', "Inside DfsWriteBlockForCacheFlush: disk_blocknum = %d\n", disk_blocknum);
    
    bcopy ((b->data + (i * DISK_BLOCKSIZE)), (db.data), DISK_BLOCKSIZE);
/*
    for (j=0; j<DISK_BLOCKSIZE; j++)
    {
      dbprintf ('b', "Inside DfsWriteBlockForCacheFlush: db.data[%d] = %d\n", j, db.data[j]);
    }
*/   
    disk_block_size_returned = DiskWriteBlock(disk_blocknum, &db);
    if (disk_block_size_returned != DISK_BLOCKSIZE)
    {
      printf("ERROR: Inside DfsWriteBlockForCacheFlush: DiskWriteBlock FAILED. disk_block_size_returned = %d\n", disk_block_size_returned);
      return DFS_FAIL;
    }
  }

  //dbprintf ('b', "Leaving DfsWriteBlockForCacheFlush\n");
  return (sb.dfs_blocksize);
}

int DfsCacheFlush()
{
  int i;
  int intrs;
  int buffer_cache_handle;
  int dfs_blocksize_returned;

  //dbprintf ('b', "Entering DfsCacheFlush\n");

  for (i=0; i<DFS_BUFFER_CACHE_NUM_SLOTS; i++)
  {
    if (read_buffer_cache_dirty(i) == 1)
    {
      if (read_buffer_cache_inuse(i) == 1)
      {
        dfs_blocksize_returned = DfsWriteBlockForCacheFlush (read_buffer_cache_dfs_blocknum(i), &buffer_cache_data[i]);
        if (dfs_blocksize_returned != sb.dfs_blocksize)
        {
          printf("ERROR: Inside DfsCacheFlush: DfsWriteBlockForCacheFlush FAILED. buffer_cache_slot = %d\n", i);
          return DFS_FAIL;
        }

        dbprintf ('b', "Inside DfsCacheFlush: Written back to disk the dfs_blocknum = %d, at dirty buffer_cache_handle = %d\n", read_buffer_cache_dfs_blocknum(i), i);
        intrs = DisableIntrs ();
        buffer_cache_tag[i].dirty = 0;    
        RestoreIntrs (intrs);
      }
    }
  }

  for (i=0; i<DFS_BUFFER_CACHE_NUM_SLOTS; i++)
  {
    intrs = DisableIntrs ();
    buffer_cache_tag[buffer_cache_handle].dfs_blocknum = 0x00000000;
    buffer_cache_tag[buffer_cache_handle].num_file_op_count = 0;
    buffer_cache_tag[buffer_cache_handle].dirty = 0;
    buffer_cache_tag[buffer_cache_handle].inuse = 0;
    RestoreIntrs (intrs);
  }

  //dbprintf ('b', "Leaving DfsCacheFlush\n");
  return DFS_SUCCESS;
}




