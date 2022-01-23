#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void makeOxygen(lock_t o_lock, lock_t o2_lock, cond_t two_o_available, cond_t two_o2_available, cond_t three_o2_available, molecule_count* mc, int* num_o2_created){
  lock_acquire(o_lock);
  lock_acquire(o2_lock);
  if(mc->o < 2){
   cond_wait(two_o_available);
  }
  Printf("O + O -> O2 \n");
  mc->o -= 2;
  mc->o2 += 1;
  if(mc->o2 >= 2 && mc->o2 < 3){
    cond_signal(two_o2_available);
  }
  else if(mc->o2 >= 3){
    cond_signal(two_o2_available);
    cond_signal(three_o2_available);
  }
  lock_release(o2_lock);
  lock_release(o_lock);
  *num_o2_created = *num_o2_created + 1;
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
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
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

  num_o2_created = 0;
  while((num_o2_created < ls->num_o2) && (ls->num_o2 != 0)){
    makeOxygen(ls->o_lock, ls->o2_lock, ls->two_o_available, ls->two_o2_available, ls->three_o2_available, mc, &num_o2_created);
  }
 
  // Now print a message to show that everything worked
  // Printf("oxygen: My PID is %d\n", Getpid());

  // Signal the semaphore to tell the original process that we're done
  // Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
