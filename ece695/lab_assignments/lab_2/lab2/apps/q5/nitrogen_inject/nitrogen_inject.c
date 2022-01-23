#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void nReady(lock_t n_lock, cond_t two_n_available, molecule_count* mc, int* num_nitrogen_injected){
  // Ready to React
  lock_acquire(n_lock);
  mc->n += 1;
  Printf("%d N\n", mc->n);
  if(mc->n >= 2){
    cond_signal(two_n_available);
  }
  lock_release(n_lock);
  *num_nitrogen_injected = *num_nitrogen_injected + 1;
}

void main (int argc, char *argv[])
{
  molecule_count *mc;        // Used to access missile codes in shared memory page
  lock_struct *ls;               // Used to get address of shared memory page
  uint32 h_mem;            // Handle to the shared memory page
  uint32 h_mem2;            // Handle to the shared memory page
  int temperature = 0;               // Used to store temp
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done
  
  int num_nitrogen_injected;

  if (argc != 5) { 
    Printf("Usage: %d",argc); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
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

  num_nitrogen_injected = 0;
  while((num_nitrogen_injected < ls->num_nitrogen) && (ls->num_nitrogen != 0)){
    nReady(ls->n_lock, ls->two_n_available, mc, &num_nitrogen_injected);
  }

  // Now print a message to show that everything worked
  // Printf("nitrogen_inject: My PID is %d\n", Getpid());

  // Signal the semaphore to tell the original process that we're done
  // Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
