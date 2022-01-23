#include "usertraps.h"
#include "misc.h"

#include "file_api.h"

#define NUMBYTES 1024

void main (int argc, char *argv[])
{
  int i;
  int return_val;
  int fd_handle;
  int num_bytes_read;
  int num_bytes_written;
  char mode;
  char big0[NUMBYTES];
  char big1[NUMBYTES];
  char big2[NUMBYTES];
  int error;

  Printf("file_api (%d): Starting the test\n", getpid());

  if (argc != 1) {
    Printf("Usage: %s\n", argv[0]);
    Exit();
  }

  for(i=0; i<NUMBYTES; i++) 
  {
    if (i < NUMBYTES/4)
      big0[i] = 1;
    else if ((i >= NUMBYTES/4) && (i < NUMBYTES/2))
      big0[i] = 2;
    else if ((i >= NUMBYTES/2) && (i < (NUMBYTES/4)*3))
      big0[i] = 3;
    else if ((i >= (NUMBYTES/4)*3) && (i < NUMBYTES))
      big0[i] = 4;
  }

  for(i=0; i<NUMBYTES; i++) 
  {
    big1[i] = i;
  }

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 1 : Opeing File in 'W' mode and Writing some data and Closing it\n");
  Printf("file_api : **************************************************************\n");

  mode = 'w';

  Printf("file_api : Testing file_open\n");
  fd_handle = file_open("test_file_1", &mode);
  Printf("file_api : fd_handle = %d\n", fd_handle);

  Printf("file_api : Testing file_write\n");
  num_bytes_written = file_write(fd_handle, big0, NUMBYTES);
  Printf("file_api : num_bytes_written = %d\n", num_bytes_written);

  Printf("file_api : Testing file_close\n");
  return_val = file_close(fd_handle);
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 2 : Opeing File in 'R' mode and Reading some data\n");
  Printf("file_api : **************************************************************\n");

  mode = 'r';

  Printf("file_api : Testing file_open\n");
  fd_handle = file_open("test_file_1", &mode);
  Printf("file_api : fd_handle = %d\n", fd_handle);

  Printf("file_api : Testing file_read\n");
  num_bytes_read = file_read(fd_handle, big2, NUMBYTES);
  Printf("file_api : num_bytes_read = %d\n", num_bytes_read);

  error = 0;
  for(i=0; i<NUMBYTES; i++) {
    if (big0[i] != big2[i]) {
      Printf("file_api: FAIL: index big0[%d] != big2[%d] (%d != %d)\n", i, i, big0[i], big2[i]);
      error = 1;
    }
  }
  if (error == 0)
    Printf("file_api : READING SUCCESSFULL\n");

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 3 : Seeking the FILE_SEEK_SET and Reading some data\n");
  Printf("file_api : **************************************************************\n");

  Printf("file_api : Testing file_seek: FILE_SEEK_SET\n");
  return_val = file_seek(fd_handle, NUMBYTES/4, FILE_SEEK_SET);
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api : Testing file_read\n");
  num_bytes_read = file_read(fd_handle, big2, NUMBYTES/2);
  Printf("file_api : num_bytes_read = %d\n", num_bytes_read);

  error = 0;
  for(i=0; i<NUMBYTES/2; i++) {
    if (big0[i + NUMBYTES/4] != big2[i]) {
      Printf("file_api: FAIL: index big0[%d] != big2[%d] (%d != %d)\n", i, i, big0[i], big2[i]);
      error = 1;
    }
  }
  if (error == 0)
    Printf("file_api : READING SUCCESSFULL\n");

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 4 : Seeking the FILE_SEEK_END and Reading some data\n");
  Printf("file_api : **************************************************************\n");

  Printf("file_api : Testing file_seek: FILE_SEEK_END\n");
  return_val = file_seek(fd_handle, (NUMBYTES/4)*3, FILE_SEEK_END);
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api : Testing file_read\n");
  num_bytes_read = file_read(fd_handle, big2, NUMBYTES/4);
  Printf("file_api : num_bytes_read = %d\n", num_bytes_read);

  error = 0;
  for(i=0; i<NUMBYTES/4; i++) {
    if (big0[i + NUMBYTES/4] != big2[i]) {
      Printf("file_api: FAIL: index big0[%d] != big2[%d] (%d != %d)\n", i, i, big0[i], big2[i]);
      error = 1;
    }
  }
  if (error == 0)
    Printf("file_api : READING SUCCESSFULL\n");

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 5 : Seeking the FILE_SEEK_CUR and Reading some data\n");
  Printf("file_api : **************************************************************\n");

  Printf("file_api : Testing file_seek: FILE_SEEK_CUR\n");
  return_val = file_seek(fd_handle, NUMBYTES/4, FILE_SEEK_CUR);
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api : Testing file_read\n");
  num_bytes_read = file_read(fd_handle, big2, NUMBYTES/4);
  Printf("file_api : num_bytes_read = %d\n", num_bytes_read);

  error = 0;
  for(i=0; i<NUMBYTES/4; i++) {
    if (big0[i + (NUMBYTES/4)*3] != big2[i]) {
      Printf("file_api: FAIL: index big0[%d] != big2[%d] (%d != %d)\n", i, i, big0[i], big2[i]);
      error = 1;
    }
  }
  if (error == 0)
    Printf("file_api : READING SUCCESSFULL\n");

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 6 : Trying to Read beyond EOF. It should give ERROR\n");
  Printf("file_api : **************************************************************\n");

  Printf("file_api : Testing file_read: Trying to Read beyond EOF. It should give ERROR\n");
  num_bytes_read = file_read(fd_handle, big2, NUMBYTES/4);
  Printf("file_api : num_bytes_read = %d\n", num_bytes_read);

  Printf("file_api : Testing file_seek: FILE_SEEK_END\n");
  return_val = file_seek(fd_handle, (NUMBYTES/4), FILE_SEEK_END);
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 7 : Currently within file position BUT Trying to Read beyond EOF. It should give WARNING\n");
  Printf("file_api : **************************************************************\n");

  Printf("file_api : Testing file_read: Currently within file position BUT Trying to Read beyond EOF. It should give WARNING\n");
  num_bytes_read = file_read(fd_handle, big2, NUMBYTES/2);
  Printf("file_api : num_bytes_read = %d\n", num_bytes_read);

  error = 0;
  for(i=0; i<NUMBYTES/4; i++) {
    if (big0[i + (NUMBYTES/4)*3] != big2[i]) {
      Printf("file_api: FAIL: index big0[%d] != big2[%d] (%d != %d)\n", i, i, big0[i], big2[i]);
      error = 1;
    }
  }
  if (error == 0)
    Printf("file_api : READING SUCCESSFULL\n");

  Printf("file_api : **************************************************************\n");
  Printf("file_api : TEST 8 : Closing and Deleting the File\n");
  Printf("file_api : **************************************************************\n");

  Printf("file_api : Testing file_close\n");
  return_val = file_close(fd_handle);
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api : Testing file_delete\n");
  return_val = file_delete("test_file_1");
  Printf("file_api : return_val = %d\n", return_val);

  Printf("file_api (%d): Ending the test\n", getpid());
}


