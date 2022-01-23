#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void makeNitrogen(lock_t n_lock, lock_t n2_lock, cond_t two_n_available, cond_t one_n2_available, molecule_count* mc, int* num_n2_created){
  lock_acquire(n_lock);
  lock_acquire(n2_lock);
  if(mc->n < 2){
   cond_wait(two_n_available);
  }
  Printf("N + N -> N2 \n");
  mc->n -= 2;
  mc->n2 += 1;
  if(mc->o2 >= 1){
    cond_signal(one_n2_available);
  }
  lock_release(n2_lock);
  lock_release(n_lock);
  *num_n2_created = *num_n2_created + 1;
}

void main (int argc, char *argv[])
{
  molecule_count *mc;        // Used to access missile codes in shared memory page
  lock_struct *ls;               // Used to get address of shared memory page
  uint32 h_mem;            // Handle to the shared memory page
  uint32 h_mem2;            // Handle to the shared memory page
  int temperature = 0;               // Used to store temp
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done

  int num_n2_created = 0;
  int num_no2_created = 0;
  int num_o2_created = 0;
  int num_o3_created = 0;

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
    Printf("Could not map  num_n2_created = 0;
 the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  // Map shared memory page into this process's memory space
  if ((ls = (lock_struct *)shmat(h_mem2)) == NULL) {
    Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  num_n2_created = 0;
  while((num_n2_created < ls->num_n2) && (ls->num_n2 != 0)){
    makeNitrogen(ls->n_lock, ls->n2_lock, ls->two_n_available, ls->one_n2_available, mc, &num_n2_created);
  }
   
  // Now print a message to show that everything worked
  // Printf("nitrogen: My PID is %d\n", Getpid());

  // Signal the semaphore to tell the original process that we're done
  // Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
