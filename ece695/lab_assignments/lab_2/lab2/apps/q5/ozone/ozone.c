#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void makeO3(lock_t o2_lock, lock_t n2_lock, cond_t three_o2_available, molecule_count* mc, int* num_o3_created, int temperature){
  lock_acquire(o2_lock);
  //lock_acquire(n2_lock);
  if(mc->o2 < 3){
    cond_wait(three_o2_available);
  }
  if((temperature > 120) || ((temperature <= 120) && (mc->n2 < 1))){
    Printf("3O2 -> 2O3 \n");
    mc->o2 -= 3;
    mc->o3 += 2;
    *num_o3_created = *num_o3_created + 1;
  }
  //lock_release(n2_lock);
  lock_release(o2_lock);
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

  num_o3_created = 0;
  while((num_o3_created < ls->num_o3) && (ls->num_o3 != 0)){
    makeO3(ls->o2_lock, ls->n2_lock, ls->three_o2_available, mc, &num_o3_created, temperature);
  }
 
  // Now print a message to show that everything worked
  // Printf("ozone: My PID is %d\n", Getpid());

  // Signal the semaphore to tell the original process that we're done
  // Printf("spawn_me: PID %d is complete.\n", Getpid());
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
