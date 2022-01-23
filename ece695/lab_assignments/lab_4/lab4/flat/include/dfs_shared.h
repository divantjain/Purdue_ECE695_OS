#ifndef __DFS_SHARED__
#define __DFS_SHARED__

typedef struct dfs_superblock {
  // STUDENT: put superblock internals here
  int valid;
  int dfs_blocksize;
  int num_dfs_blocks;
  uint32 inode_array_start_dfs_block_num;
  int num_inode;
  uint32 free_block_vector_start_dfs_block_num;
} dfs_superblock;

#define DFS_BLOCKSIZE 1024  // Must be an integer multiple of the disk blocksize

typedef struct dfs_block {
  char data[DFS_BLOCKSIZE];
} dfs_block;

#define DFS_INODE_MAX_FILENAME_LENGTH (72)
#define DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS (10)
#define DFS_BLOCK_POINTER_SIZE (4)
#define DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS (DFS_BLOCKSIZE / DFS_BLOCK_POINTER_SIZE)
#define DFS_INODE_NUM_DOUBLE_INDIRECT_ADDRESSED_BLOCKS ((DFS_BLOCKSIZE / DFS_BLOCK_POINTER_SIZE) * DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS)
#define DFS_INODE_NUM_INDIRECT_ADDRESSED_BLOCKS (DFS_INODE_NUM_SINGLE_INDIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_DOUBLE_INDIRECT_ADDRESSED_BLOCKS)
#define DFS_INODE_MAX_VIRTUAL_BLOCKNUM (DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS + DFS_INODE_NUM_INDIRECT_ADDRESSED_BLOCKS - 1)

typedef struct dfs_inode {
  // STUDENT: put inode structure internals here
  // IMPORTANT: sizeof(dfs_inode) MUST return 128 in order to fit in enough
  // inodes in the filesystem (and to make your life easier).  To do this, 
  // adjust the maximumm length of the filename until the size of the overall inode 
  // is 128 bytes.
  int inuse;
  char filename [DFS_INODE_MAX_FILENAME_LENGTH];
  int current_file_size;
  uint32 direct_addressed_blocks [DFS_INODE_NUM_DIRECT_ADDRESSED_BLOCKS];
  uint32 single_indirect_addressed_blocks_dfs_block_num;
  uint32 double_indirect_addressed_blocks_dfs_block_num;  
} dfs_inode;

#define DFS_MAX_FILESYSTEM_SIZE 0x10000000  // 64MB 
#define DFS_MAX_NUM_BLOCKS (DFS_MAX_FILESYSTEM_SIZE / DFS_BLOCKSIZE)

#define DFS_BITS_PER_BYTE (8)
#define DFS_FBV_BYTES_PER_ENTRY (4)
#define DFS_FBV_BITS_PER_ENTRY (DFS_FBV_BYTES_PER_ENTRY * DFS_BITS_PER_BYTE)
#define DFS_FBV_MAX_NUM_WORDS (DFS_MAX_NUM_BLOCKS / DFS_FBV_BITS_PER_ENTRY)
#define DFS_INODE_MAX_NUM_FILESYSTEM_BLOCKS (16)
#define DFS_INODE_MAX_NUM (128)

#define DFS_SUPERBLOCK_PHYSICAL_BLOCKNUM 1 // Where to write superblock on the disk

typedef struct buffer_cache_node {
  int inuse;
  int dirty;
  int dfs_blocknum;
  int num_file_op_count;
} buffer_cache_node;

#define DFS_BUFFER_CACHE_NUM_SLOTS (16)

#define DFS_FAIL -1
#define DFS_SUCCESS 1



#endif
