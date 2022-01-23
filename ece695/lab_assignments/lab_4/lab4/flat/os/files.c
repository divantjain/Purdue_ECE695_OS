#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "dfs.h"
#include "files.h"
#include "synch.h"

/*
  dbprintf ('d', "Entering \n");
  dbprintf ('d', "Leaving \n");  
*/

static file_descriptor fds[FILE_MAX_OPEN_FILES]; // all fds

int read_fd_inuse (uint32 fd_handle)
{
  int intrs;
  int inuse;

  dbprintf ('d', "Entering read_fd_inuse\n");
  
  intrs = DisableIntrs ();
  inuse = fds[fd_handle].inuse;
  RestoreIntrs (intrs);

  dbprintf ('d', "Leaving read_fd_inuse: fd_handle.inuse = %d\n", inuse);

  return inuse;
}

uint32 allocate_free_file_descriptor ()
{
  int i;
  int hit;
  uint32 fd_handle;

  dbprintf ('d', "Entering allocate_free_file_descriptor\n");
  
  hit = 0;
  for (i=0; i<FILE_MAX_OPEN_FILES; i++)
  {
    if(read_fd_inuse(i) == 0)
    {
      hit = 1;
      fd_handle = i;
      break;
    }
  }
  
  if (hit == 0)
  {
    printf("ERROR: Inside allocate_free_file_descriptor: No Free File Descriptor\n");
    return FILE_FAIL;
  }
  else
  {
    dbprintf ('d', "Leaving allocate_free_file_descriptor: Allocated fd_handle = %d\n", fd_handle);  
    return fd_handle;
  }
}

int compare_filename (char *input_filename, char *inode_filename)
{
  int i;
  int mismatch;

  //dbprintf ('d', "Inside compare_filename\n");

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
    return FILE_FAIL;
  }
  else
  {
    //dbprintf ('d', "Leaving compare_filename, with Success\n");
    return FILE_SUCCESS;
  }
}

void copy_filename_to_fd (char *input_filename, char *fd_filename)
{
  int i;

  dbprintf ('d', "Entering copy_filename_to_fd\n");

  for (i=0; i<DFS_INODE_MAX_FILENAME_LENGTH; i++) 
  {
    fd_filename[i] = 0;
  }
  
  for (i=0; input_filename[i]!='\0'; i++) 
  {
    fd_filename[i] = input_filename[i];
  }  
  dbprintf ('d', "Leaving copy_filename_to_fd\n");
}

void assign_fd (uint32 fd_handle, char *filename, uint32 inode, int eof, char mode, int current_position, int pid)
{
  dbprintf ('d', "Entering assign_fd\n");

  copy_filename_to_fd(filename, fds[fd_handle].filename);
  fds[fd_handle].inode = inode;
  fds[fd_handle].eof = eof;
  fds[fd_handle].mode = mode;
  fds[fd_handle].current_position = current_position;
  fds[fd_handle].pid = pid;

  dbprintf ('d', "Leaving assign_fd: Assigned fields of fd_handle = %d\n", fd_handle);
}

uint32 get_fd_handle_for_existing_filename (char *filename)
{
  int i;
  int hit;
  uint32 fd_handle;

  dbprintf ('d', "Entering get_fd_handle_for_existing_filename\n");
  hit = 0;
  for(i=0; i<FILE_MAX_OPEN_FILES; i++)
  {
    if (read_fd_inuse(i) == 1)
    {
      if (compare_filename(filename, fds[i].filename) == FILE_SUCCESS)
      {
        hit = 1;
        fd_handle = i;
        break;
      }
    }
  }

  if (hit == 0)
  {
    dbprintf ('d', "Leaving get_fd_handle_for_existing_filename: filename NOT found at fd_handle = %d.\n", fd_handle);
    return FILE_FAIL;
  }
  else
  {
    dbprintf ('d', "Leaving get_fd_handle_for_existing_filename: filename found at fd_handle = %d\n", fd_handle);
    return fd_handle;
  }
}

int pid_authorization_check (uint32 fd_handle)
{
  dbprintf ('d', "Entering pid_authorization_check\n");

  if (fds[fd_handle].pid == GetCurrentPid())
  {
    dbprintf ('d', "Leaving pid_authorization_check SUCCEED for fd_handle = %d\n", fd_handle);
    return FILE_SUCCESS;  
  }
  else
  {
    dbprintf ('d', "ERROR: pid_authorization_check FAILED for fd_handle = %d\n", fd_handle);
    return FILE_FAIL;
  }
}

void FileModuleInit ()
{
  int i;
  int intrs;
  char filename [DFS_INODE_MAX_FILENAME_LENGTH];
  int inode;
  int eof;
  char mode;
  int current_position;
  int pid;  

  dbprintf ('d', "Entering FileModuleInit\n");  

  bzero(filename, DFS_INODE_MAX_FILENAME_LENGTH);
  inode = -1;
  eof = 0;
  mode = 'r';
  current_position = 0;
  pid = -1;

  for (i=0; i<FILE_MAX_OPEN_FILES; i++)
  {
    assign_fd (i, filename, inode, eof, mode, current_position, pid);

    intrs = DisableIntrs ();
    fds[i].inuse = 0;
    RestoreIntrs (intrs);
  }

  dbprintf ('d', "Leaving FileModuleInit\n");  
}

// STUDENT: put your file-level functions here
uint32 FileOpen(char *filename, char *mode) 
{
  int intrs;
  uint32 fd_handle;
  uint32 inode_handle;
  int eof;
  int current_position;
  int pid;
  int return_val;

  dbprintf ('d', "Entering FileOpen\n");

  // 1) Get the current PID
  pid = GetCurrentPid();

  // 2) Check the mode of file to be opened
  // If mode is "r" and file not existed previously, Return with Error
  if(*mode == 'r')
  {
    inode_handle = DfsInodeFilenameExists(filename);
    if (inode_handle == DFS_FAIL)
    {
      printf("ERROR: Inside FileOpen: Mode was READ and file was NOT opened before.\n");
      return FILE_FAIL;
    }
  }
  // If mode is "w"
  // If mode is "w" AND inode was NOT opened before, open an inode
  // If mode is "w" AND inode was opened before, delete the previous inode, open a new inode
  else if (*mode == 'w')
  {
    inode_handle = DfsInodeFilenameExists(filename);
    if (inode_handle == DFS_FAIL)
    {
      // 2.1) Allocate new inode
      inode_handle = DfsInodeOpen(filename);
      if (inode_handle == DFS_FAIL)
      {
        printf("ERROR: Inside FileOpen: DfsInodeOpen FAILED.\n");
        return FILE_FAIL;
      }      
    }
    else
    {
      // 2.1) Delete previous inode
      return_val = DfsInodeDelete (inode_handle);
      if (return_val == DFS_FAIL)
      {
        printf("ERROR: Inside FileOpen: DfsInodeDelete FAILED.\n");
        return FILE_FAIL;
      }

      // 2.2) Allocate new inode
      inode_handle = DfsInodeOpen(filename);
      if (inode_handle == DFS_FAIL)
      {
        printf("ERROR: Inside FileOpen: DfsInodeOpen FAILED.\n");
        return FILE_FAIL;
      }
    }
  }
  else
  {
    printf("ERROR: Inside FileOpen: Wrong value of mode.\n");
    return FILE_FAIL;
  }

  // 3) Allocate new fd
  fd_handle = allocate_free_file_descriptor();
  if (fd_handle == FILE_FAIL)
  {
    printf("ERROR: Inside FileOpen: allocate_free_file_descriptor FAILED.\n");
    return FILE_FAIL;
  }
  
  // 4) Initialize eof and current_position
  eof = 0;
  current_position = 0;    

  // 5) Assign the fields of fd
  dbprintf ('d', "Inside FileOpen: fd_handle = %d, inode_handle = %d, eof = %d, (*mode) = %c, current_position = %d, pid = %d\n", fd_handle, inode_handle, eof, (*mode), current_position, pid);
  assign_fd (fd_handle, filename, inode_handle, eof, (*mode), current_position, pid);
  
  // 6) Set the inuse bit of fd
  intrs = DisableIntrs ();
  fds[fd_handle].inuse = 1;
  RestoreIntrs (intrs);
  
  dbprintf ('d', "Leaving FileOpen: Opened the file in WRITE mode at fd_handle = %d\n", fd_handle);
  return fd_handle;
}

int FileClose(uint32 handle)
{
  int return_val;
  int intrs;
  char filename [DFS_INODE_MAX_FILENAME_LENGTH];
  int inode;
  int eof;
  char mode;
  int current_position;
  int pid;  

  dbprintf ('d', "Entering FileClose\n");
  
  // 1) Check the fd is inuse
  if (read_fd_inuse(handle) == 0)
  {
    printf("ERROR: Inside FileClose: fd_insue = 0.\n");
    return FILE_FAIL;
  }

  // 2) PID Authorization check
  return_val = pid_authorization_check(handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside FileClose: pid_authorization_check FAILED.\n");
    return FILE_FAIL;
  }

  // 3) Clear the inuse bit of fd
  intrs = DisableIntrs ();
  fds[handle].inuse = 0;
  RestoreIntrs (intrs);

  // 4) Initialize the fields of fd
  bzero(filename, DFS_INODE_MAX_FILENAME_LENGTH);
  inode = -1;
  eof = 0;
  mode = 'r';
  current_position = 0;
  pid = -1;
  assign_fd (handle, filename, inode, eof, mode, current_position, pid);

  dbprintf ('d', "Leaving FileClose: Closed the handle = %d\n", handle);
  return FILE_SUCCESS;
}

int FileDelete(char *filename) 
{
  int inode_handle;
  int return_val;
  
  dbprintf ('d', "Entering FileDelete\n");

  // 1) Get the inode handle of file with filename
  inode_handle = DfsInodeFilenameExists(filename);
  if (inode_handle == DFS_FAIL)
  {
    printf("ERROR: Inside FileDelete: DfsInodeFilenameExists FAILED. filename not found in any inode.\n");
    return FILE_FAIL;
  }

  // 2) Delete the inode associated with fd_handle
  return_val = DfsInodeDelete (inode_handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside FileDelete: DfsInodeDelete FAILED.\n");
    return FILE_FAIL;
  }

  dbprintf ('d', "Leaving FileDelete. Deleted the inode_handle = %d\n", inode_handle);
  return FILE_SUCCESS;
}

int FileRead(uint32 handle, void *mem, int num_bytes) 
{
  int return_val;
  int start_byte;
  int num_bytes_returned;
  int current_file_size;

  dbprintf ('d', "Entering FileRead\n");
  
  // 1) PID Authorization check
  return_val = pid_authorization_check(handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside FileRead: pid_authorization_check FAILED.\n");
    return FILE_FAIL;
  }
  
  // 2) Error on Lower and Max limits of num_bytes
  if ((num_bytes < 0) || (num_bytes > FILE_MAX_READWRITE_BYTES))
  {
    printf("ERROR: Inside FileRead: (num_bytes < 0) || (num_bytes > FILE_MAX_READWRITE_BYTES).\n");
    return FILE_FAIL;
  }

  // 3) Error if EOF is Set
  if (fds[handle].eof == 1)
  {
    printf("ERROR: Inside FileRead: EOF is set.\n");
    return FILE_FAIL;
  }

  // 4) Logic for Reading the DFS Blocks
  current_file_size = DfsInodeFilesize(fds[handle].inode);
  if (current_file_size == DFS_FAIL)
  {
    printf("ERROR: Inside FileRead: DfsInodeFilesize FAILED.\n");
    return FILE_FAIL;
  }
  start_byte = fds[handle].current_position;
  if (num_bytes > ((current_file_size + 1) - start_byte))
  {
    printf("WARNING: Inside FileRead: The num_bytes provided exceeds the current_max_file_size. Reading the bytes till EOF\n");
    num_bytes = (current_file_size + 1) - start_byte;
  }
  num_bytes_returned = DfsInodeReadBytes (fds[handle].inode, (char *)(mem), start_byte, num_bytes);
  if (num_bytes_returned == DFS_FAIL)
  {
    printf("ERROR: Inside FileRead: DfsInodeReadBytes FAILED. num_bytes_returned = %d\n", num_bytes_returned);
    return FILE_FAIL;
  }

  // 5) Logic for current_position
  fds[handle].current_position = (fds[handle].current_position + num_bytes_returned);

  // 6) Logic for eof
  current_file_size = DfsInodeFilesize(fds[handle].inode);
  if (current_file_size == DFS_FAIL)
  {
    printf("ERROR: Inside FileRead: DfsInodeFilesize FAILED.\n");
    return FILE_FAIL;
  }
  if (fds[handle].current_position > current_file_size)
  {
    printf("WARNING: Inside FileRead: The EOF has been Set\n");
    fds[handle].eof = 1;
  }
  
  dbprintf ('d', "Leaving FileRead: num_bytes_returned = %d\n", num_bytes_returned);
  return num_bytes_returned;
}

int FileWrite(uint32 handle, void *mem, int num_bytes) 
{
  int return_val;
  int num_bytes_returned;
  int start_byte;

  dbprintf ('d', "Entering FileWrite\n");
  
  // 1) PID Authorization check
  return_val = pid_authorization_check(handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside FileWrite: pid_authorization_check FAILED.\n");
    return FILE_FAIL;
  }

  // 2) Error on Lower and Max limits of num_bytes
  if ((num_bytes < 0) || (num_bytes > FILE_MAX_READWRITE_BYTES))
  {
    printf("ERROR: Inside FileWrite: (num_bytes < 0) || (num_bytes > FILE_MAX_READWRITE_BYTES).\n");
    return FILE_FAIL;
  }

  // 3) Error if mode is "r"
  if (fds[handle].mode == 'r')
  {
    printf("ERROR: Inside FileWrite: File mode is READ.\n");
    return FILE_FAIL;
  }

  // 4) Logic for Writing to the DFS Blocks, 1 at a time
  start_byte = fds[handle].current_position;
  num_bytes_returned = DfsInodeWriteBytes (fds[handle].inode, (char *)(mem), start_byte, num_bytes);
  if (num_bytes_returned == DFS_FAIL)
  {
    printf("ERROR: Inside FileWrite: DfsInodeWriteBytes FAILED. num_bytes_returned = %d\n", num_bytes_returned);
    return FILE_FAIL;
  }
    
  // 5) Logic for current_position
  fds[handle].current_position = (fds[handle].current_position + num_bytes_returned);

  dbprintf ('d', "Leaving FileWrite: num_bytes_returned = %d\n", num_bytes_returned);
  return num_bytes_returned;
}

int FileSeek(uint32 handle, int num_bytes, int from_where) 
{
  int return_val;
  int current_file_size;

  dbprintf ('d', "Entering FileSeek\n");
  
  // 1) PID Authorization check
  return_val = pid_authorization_check(handle);
  if (return_val == DFS_FAIL)
  {
    printf("ERROR: Inside FileSeek: pid_authorization_check FAILED.\n");
    return FILE_FAIL;
  }

  // 2) Error on Lower and Max limits of num_bytes
  if ((num_bytes < 0) || (num_bytes > FILE_MAX_READWRITE_BYTES))
  {
    printf("ERROR: Inside FileSeek: (num_bytes < 0) || (num_bytes > FILE_MAX_READWRITE_BYTES).\n");
    return FILE_FAIL;
  }

  // 3) Logic for Seeking or current_position
  if (from_where == FILE_SEEK_SET)
  {
    fds[handle].current_position = 0 + num_bytes;
  }
  else if (from_where == FILE_SEEK_END)
  {
    current_file_size = DfsInodeFilesize (handle);
    if (current_file_size == DFS_FAIL)
    {
      printf("ERROR: Inside FileSeek: DfsInodeFilesize FAILED.\n");
      return FILE_FAIL;
    }
    fds[handle].current_position = (current_file_size + 1) - num_bytes;
  }
  else if (from_where == FILE_SEEK_CUR)
  {
    fds[handle].current_position = fds[handle].current_position + num_bytes;
  }
  else
  {
    printf("ERROR: Inside FileSeek: Wrong value of from_where. from_where = %d\n", from_where);
    return FILE_FAIL;
  }

  // 4) Logic for eof
  fds[handle].eof = 0;

  dbprintf ('d', "Leaving FileSeek: fds[handle].current_position = %d\n", fds[handle].current_position);
  return FILE_SUCCESS;
}


