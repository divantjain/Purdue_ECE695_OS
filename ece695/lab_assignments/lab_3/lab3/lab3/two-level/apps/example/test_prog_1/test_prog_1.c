#include "usertraps.h"
#include "misc.h"

#define MEM_MAX_VIRTUAL_ADDRESS (0xfffff)

void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done
  
  int value;
  int *ptr;

  if (argc != 2) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  // Now print a message to show that everything worked
  Printf("test_prog_1 (%d): Starting Test!\n", getpid());

  ptr = (int *)(0x1 + MEM_MAX_VIRTUAL_ADDRESS);
  Printf("test_prog_1 (%d): Trying to Access memory beyond the maximum virtual address. Program should exit after this.\n", getpid());
  Printf("test_prog_1 (%d): Access Address = %d\n", getpid(), (int)(ptr));
  value = *ptr;
  Printf("test_prog_1 (%d): ERROR: Program didn't exit, after acceccing: ptr = %d, value = %d\n", getpid(), (int)(ptr), value);
  
  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("test_prog_1 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("test_prog_1 (%d): Done!\n", getpid());
}
