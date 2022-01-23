#ifndef __FILES_SHARED__
#define __FILES_SHARED__

/* Define variables used by the file system (not DFS) here */

#define FILE_SEEK_SET 1
#define FILE_SEEK_END 2
#define FILE_SEEK_CUR 3

#define FILE_MAX_FILENAME_LENGTH (DFS_INODE_MAX_FILENAME_LENGTH) // Change this. This is set to 1 just to compile in the beginning of the lab

#define FILE_MAX_READWRITE_BYTES (4096) // Again change this. This is set to 1 just to compile in the beginning of the lab

typedef struct file_descriptor {
  // STUDENT: put file descriptor info here
  int inuse;
  char filename [FILE_MAX_FILENAME_LENGTH];
  uint32 inode;
  int eof;
  char mode;
  int current_position;
  int pid;
} file_descriptor;

#define FILE_FAIL -1
#define FILE_EOF -1
#define FILE_SUCCESS 1

#endif
