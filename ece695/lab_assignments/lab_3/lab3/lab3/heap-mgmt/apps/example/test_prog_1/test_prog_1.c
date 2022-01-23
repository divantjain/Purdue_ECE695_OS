#include "usertraps.h"
#include "misc.h"

void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  int *ptr1;
  int *ptr2;
  int *ptr3;
  int *ptr4;
  int *ptr5;

  int size1;
  int size2;
  int size3;
  int size4;
  int size5;

  if (argc != 2) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  // Now print a message to show that everything worked
  Printf("hello_world (%d): Hello world!\n", getpid());

  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 1: requests memory 34 B\n");
  ptr1 = (int *)malloc(34);
  Printf("Got the ptr1: %d\n", (int)ptr1);
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 2: requests memory 66 B\n");
  ptr2 = (int *)malloc(66);
  Printf("Got the ptr2: %d\n", (int)ptr2);
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 3: requests memory 35 B\n");
  ptr3 = (int *)malloc(35);
  Printf("Got the ptr3: %d\n", (int)ptr3);
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 4: requests memory 67 B\n");
  ptr4 = (int *)malloc(67);
  Printf("Got the ptr4: %d\n", (int)ptr4);
  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 2: releases its memory\n");
  size1 = mfree(ptr1);
  Printf("Got the size1: %d\n", size1);
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 4: releases its memory\n");
  size2 = mfree(ptr2);
  Printf("Got the size2: %d\n", size2);
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 1: releases its memory\n");
  size3 = mfree(ptr3);
  Printf("Got the size3: %d\n", size3);
  Printf("---------------------------------------------------------------------\n");
  Printf("Program A: 3: releases its memory\n");
  size4 = mfree(ptr4);
  Printf("Got the size4: %d\n", size4);
  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("hello_world (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("hello_world (%d): Done!\n", getpid());
}
