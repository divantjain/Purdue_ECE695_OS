#include "usertraps.h"
#include "misc.h"

void count_large_number (int count_val)
{
  int count;
  for(count=0; count<count_val; count++);  
}

void main (int argc, char *argv[])
{
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done
  int spawned_proc_id;

  if (argc != 3) { 
    Printf("Usage: %s <handle_to_procs_completed_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  s_procs_completed = dstrtol(argv[1], NULL, 10);

  spawned_proc_id = dstrtol(argv[2], NULL, 10);

  // Now print a message to show that everything worked
  //Printf("test_prog_5 (%d): Starting Test!: spawned_proc_id: %d\n", getpid(), spawned_proc_id);

  Printf("test_prog_5 (%d): Starting Counting: spawned_proc_id: %d\n", getpid(), spawned_proc_id);
  count_large_number(100000);
  Printf("test_prog_5 (%d): Ended Counting: spawned_proc_id: %d\n", getpid(), spawned_proc_id);

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("test_prog_5 (%d): Bad semaphore s_procs_completed (%d)! spawned_proc_id: %d\n", getpid(), s_procs_completed, spawned_proc_id);
    Exit();
  }

  //Printf("test_prog_5 (%d): Done!: spawned_proc_id: %d\n", getpid(), spawned_proc_id);
}
