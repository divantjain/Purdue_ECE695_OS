#include "ostraps.h"
#include "dlxos.h"
#include "traps.h"
#include "disk.h"
#include "dfs.h"

#define NUMBYTES 1024

void RunOSTests() 
{
  // STUDENT: run any os-level tests here

  uint32 inode;
  int current_file_size;
  int a;
  char big0[NUMBYTES];
  char big1[NUMBYTES];
  char big2[NUMBYTES];
  char big3[NUMBYTES*4];
  char big4[NUMBYTES*4];
  char big5[NUMBYTES*4];

  for(a=0; a<NUMBYTES; a++) 
  {
    big0[a] = 0;
  }

  for(a=0; a<NUMBYTES; a++) 
  {
    big1[a] = a;
  }

  printf("runostests: Starting OS Tests\n");

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 1: Inode Open, Delete, FileNameEsists\n");
  printf("runostests: *************************************************************\n");

  printf("runostests: TESTING DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeOpen Again\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeFilenameExists\n");
  inode = DfsInodeFilenameExists("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: TESTING DfsInodeOpen for another file\n");
  inode = DfsInodeOpen("ece695-file-2");
  printf("runostests: ece695-file-2 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeDelete for another file\n");
  DfsInodeDelete(inode);

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 2: Direct Addressed Read Write\n");
  printf("runostests: *************************************************************\n");

  printf("runostests: DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big1, 0*NUMBYTES, NUMBYTES);

  printf("runostests: TESTING DfsInodeFilesize\n");
  current_file_size = DfsInodeFilesize(inode);
  printf("runostests: TESTING DfsInodeFilesize: DfsInodeFilesize = %d\n", current_file_size);

  printf("runostests: TESTING DfsInodeReadBytes\n");
  DfsInodeReadBytes(inode, big2, 0*NUMBYTES, NUMBYTES);

  for(a=0; a<NUMBYTES; a++) {
    if (big1[a] != big2[a]) {
      printf("runostests: FAIL: index big1[%d] != big2[%d] (%d != %d)\n", a, a, big1[a], big2[a]);
      GracefulExit();
    }
  }
  printf("runostests: ece695-file-1 ops worked!\n");

  printf("runostests: DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 3: Single-Indirect Addressed Read Write\n");
  printf("runostests: *************************************************************\n");

  printf("runostests: DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  for(a=0; a<13; a++) 
  { 
    printf("runostests: TESTING DfsInodeWriteBytes: a = %d\n", a);
    DfsInodeWriteBytes(inode, big1, a*NUMBYTES, NUMBYTES);
  }

  printf("runostests: TESTING DfsInodeFilesize\n");
  current_file_size = DfsInodeFilesize(inode);
  printf("runostests: TESTING DfsInodeFilesize: DfsInodeFilesize = %d\n", current_file_size);

  printf("runostests: DfsInodeReadBytes\n");
  DfsInodeReadBytes(inode, big2, (a-1)*NUMBYTES, NUMBYTES);

  for(a=0; a<NUMBYTES; a++) {
    if (big1[a] != big2[a]) {
      printf("runostests: FAIL: index big1[%d] != big2[%d] (%d != %d)\n", a, a, big1[a], big2[a]);
      GracefulExit();
    }
  }
  printf("runostests: ece695-file-1 ops worked!\n");

  printf("runostests: DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 4: Double-Indirect Addressed Read Write\n");
  printf("runostests: *************************************************************\n");

  printf("runostests: DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  for(a=0; a<270; a++) 
  { 
    printf("runostests: TESTING DfsInodeWriteBytes: a = %d\n", a);
    DfsInodeWriteBytes(inode, big1, a*NUMBYTES, NUMBYTES);
  }

  printf("runostests: TESTING DfsInodeFilesize\n");
  current_file_size = DfsInodeFilesize(inode);
  printf("runostests: TESTING DfsInodeFilesize: DfsInodeFilesize = %d\n", current_file_size);

  printf("runostests: TESTING DfsInodeReadBytes\n");
  DfsInodeReadBytes(inode, big2, (a-1)*NUMBYTES, NUMBYTES);

  for(a=0; a<NUMBYTES; a++) {
    if (big1[a] != big2[a]) {
      printf("runostests: FAIL: index big1[%d] != big2[%d] (%d != %d)\n", a, a, big1[a], big2[a]);
      GracefulExit();
    }
  }
  printf("runostests: ece695-file-1 ops worked!\n");

  printf("runostests: DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 5: Non Aligned Read and Write at DISK Size Boundary\n");
  printf("runostests: *************************************************************\n");

  printf("runostests: DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big0, 0*NUMBYTES, NUMBYTES);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big1, NUMBYTES/2, NUMBYTES/2);

  printf("runostests: TESTING DfsInodeFilesize\n");
  current_file_size = DfsInodeFilesize(inode);
  printf("runostests: TESTING DfsInodeFilesize: DfsInodeFilesize = %d\n", current_file_size);

  printf("runostests: TESTING DfsInodeReadBytes\n");
  DfsInodeReadBytes(inode, big2, NUMBYTES/2, NUMBYTES/2);

  for(a=0; a<NUMBYTES/2; a++) {
    if (big1[a] != big2[a]) {
      printf("runostests: FAIL: index big1[%d] != big2[%d] (%d != %d)\n", a, a, big1[a], big2[a]);
      GracefulExit();
    }
  }
  printf("runostests: ece695-file-1 ops worked!\n");

  printf("runostests: DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 6: Non Aligned Read and Write at Random Start Address\n");
  printf("runostests: *************************************************************\n");

  printf("runostests: DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big0, 0*NUMBYTES, NUMBYTES);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big1, NUMBYTES/4, NUMBYTES/2);

  printf("runostests: TESTING DfsInodeFilesize\n");
  current_file_size = DfsInodeFilesize(inode);
  printf("runostests: TESTING DfsInodeFilesize: DfsInodeFilesize = %d\n", current_file_size);

  printf("runostests: TESTING DfsInodeReadBytes\n");
  DfsInodeReadBytes(inode, big2, NUMBYTES/4, NUMBYTES/2);

  for(a=0; a<NUMBYTES/2; a++) {
    if (big1[a] != big2[a]) {
      printf("runostests: FAIL: index big1[%d] != big2[%d] (%d != %d)\n", a, a, big1[a], big2[a]);
      GracefulExit();
    }
  }
  printf("runostests: ece695-file-1 ops worked!\n");

  printf("runostests: DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: *************************************************************\n");
  printf("runostests: TEST 7: Non Aligned Read and Write at Random Start Address, spanning multiple dfs blocks\n");
  printf("runostests: *************************************************************\n");

  for(a=0; a<NUMBYTES*4; a++) 
  {
    big5[a] = 0;
  }

  for(a=0; a<NUMBYTES*4; a++) 
  {
    big3[a] = a;
  }

  printf("runostests: DfsInodeOpen\n");
  inode = DfsInodeOpen("ece695-file-1");
  printf("runostests: ece695-file-1 open, inode = %d\n", inode);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big5, 0*NUMBYTES, NUMBYTES*4);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big5, 4*NUMBYTES, NUMBYTES*4);

  printf("runostests: TESTING DfsInodeWriteBytes\n");
  DfsInodeWriteBytes(inode, big3, NUMBYTES/4, NUMBYTES*4);

  printf("runostests: TESTING DfsInodeFilesize\n");
  current_file_size = DfsInodeFilesize(inode);
  printf("runostests: TESTING DfsInodeFilesize: DfsInodeFilesize = %d\n", current_file_size);

  printf("runostests: TESTING DfsInodeReadBytes\n");
  DfsInodeReadBytes(inode, big4, NUMBYTES/4, NUMBYTES*4);

  for(a=0; a<NUMBYTES/2; a++) {
    if (big3[a] != big4[a]) {
      printf("runostests: FAIL: index big3[%d] != big4[%d] (%d != %d)\n", a, a, big3[a], big4[a]);
      GracefulExit();
    }
  }
  printf("runostests: ece695-file-1 ops worked!\n");

  printf("runostests: DfsInodeDelete\n");
  DfsInodeDelete(inode);

  printf("runostests: Ending OS Tests\n");
}

