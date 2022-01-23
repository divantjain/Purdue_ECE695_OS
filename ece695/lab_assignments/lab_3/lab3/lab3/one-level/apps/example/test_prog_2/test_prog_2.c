#include "usertraps.h"
#include "misc.h"

#define MEM_PAGESIZE (0x1000)

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
  Printf("test_prog_2 (%d): Starting Test!\n", getpid());

  ptr = (int *)(MEM_PAGESIZE * 4);
  Printf("test_prog_2 (%d): Access memory inside the virtual address space, but outside of currently allocated pages. Program should enter Page Fault Handler after this, and Exit thereafter.\n", getpid());
  Printf("test_prog_2 (%d): Access Address = %d\n", getpid(), (int)(ptr));
  value = *ptr;
  Printf("test_prog_2 (%d): ERROR: Program didn't exit, after acceccing: ptr = %d, value = %d\n", getpid(), (int)(ptr), value);
  
  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("test_prog_2 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("test_prog_2 (%d): Done!\n", getpid());
}

