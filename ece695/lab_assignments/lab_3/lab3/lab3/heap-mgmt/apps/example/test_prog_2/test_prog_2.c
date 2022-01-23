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
  Printf("Tesing malloc 1\n");
  ptr1 = (int *)malloc(30);
  Printf("ptr1: %d\n", (int)ptr1);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing malloc 2\n");
  ptr2 = (int *)malloc(30);
  Printf("ptr2: %d\n", (int)ptr2);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing malloc 3\n");
  ptr3 = (int *)malloc(30);
  Printf("ptr3: %d\n", (int)ptr3);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing malloc 4\n");
  ptr4 = (int *)malloc(30);
  Printf("ptr4: %d\n", (int)ptr4);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing malloc 5\n");
  ptr5 = (int *)malloc(30);
  Printf("ptr5: %d\n", (int)ptr5);
  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing mfree 1\n");
  size1 = mfree(ptr1);
  Printf("size1: %d\n", size1);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing mfree 2\n");
  size2 = mfree(ptr2);
  Printf("size2: %d\n", size2);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing mfree 3\n");
  size3 = mfree(ptr3);
  Printf("size3: %d\n", size3);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing mfree 4\n");
  size4 = mfree(ptr4);
  Printf("size4: %d\n", size4);
  Printf("---------------------------------------------------------------------\n");
  Printf("Tesing mfree 5\n");
  size5 = mfree(ptr5);
  Printf("size5: %d\n", size5);
  Printf("---------------------------------------------------------------------\n");
  Printf("---------------------------------------------------------------------\n");

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("hello_world (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("hello_world (%d): Done!\n", getpid());
}
