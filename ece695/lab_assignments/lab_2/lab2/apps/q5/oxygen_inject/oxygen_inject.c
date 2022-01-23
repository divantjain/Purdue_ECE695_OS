#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void oReady(lock_t o_lock, cond_t two_o_available, molecule_count* mc, int* num_oxygen_injected){
  // Ready to React
  lock_acquire(o_lock);
  mc->o += 1;
  Printf("%d O\n", mc->o);
  if(mc->o >= 2){
    cond_signal(two_o_available);
  }
  lock_release(o_lock);
  *num_oxygen_injected = *num_oxygen_injected + 1;
}

void main (int argc, char *argv[])
{
  molecule_count *mc;        // Used to access missile codes in shared memory page
  lock_struct *ls;               // Used to get address of shared memory page
  uint32 h_mem;            // Handle to the shared memory page
  uint32 h_mem2;            // Handle to the shared memory page
  int temperature = 0;               // Used to store temp
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  int num_oxygen_injected;

  if (argc != 5) { 
    Printf("Usage:  %d",argc); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);
  temperature = dstrtol(argv[3], NULL, 10);
  h_mem2 = dstrtol(argv[4], NULL, 10); // The "10" means base 10

  // Map shared memory page into this process's memory space
  if ((mc = (molecule_count *)shmat(h_mem)) == NULL) {
    Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  // Map shared memory page into this process's memory space
  if ((ls = (lock_struct *)shmat(h_mem2)) == NULL) {
    Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  num_oxygen_injected = 0;
  while((num_oxygen_injected < ls->num_oxygen) && (ls->num_oxygen != 0)){
    oReady(ls->o_lock, ls->two_o_available, mc, &num_oxygen_injected);
  }

  // Now print a message to show that everything worked
  // Printf("oxygen_inject: My PID is %d\n", Getpid());

  // Signal the semaphore to tell the original process that we're done
  // Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
