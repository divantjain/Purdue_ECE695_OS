#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{ 
  int numprocs = 0;
  int num_oxygen = 0;               // Used to store number of O
  int num_nitrogen = 0;               // Used to store number of N
  int temperature = 0;               // Used to store temp
  molecule_count *mc;               // Used to get address of shared memory page
  lock_struct *ls;               // Used to get address of shared memory page
  uint32 h_mem;                   // Used to hold handle to shared memory page
  uint32 h_mem2;                   // Used to hold handle to shared memory page
  sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed

  char h_mem_str[10];             // Used as command-line argument to pass mem_handle to new processes
  char h_mem2_str[10];             // Used as command-line argument to pass mem_handle to new processes
  char temperature_str[10]; // Used as command-line argument to pass temperature
  char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes

  int num_o2 = 0;               // Used to store number of O2
  int num_n2 = 0;               // Used to store number of N2
  int num_o3 = 0;               // Used to store number of O3
  int num_no2 = 0;               // Used to store number of NO2

  if (argc != 4) {
    Printf("Usage: "); Printf(argv[0]); Printf(" <number of processes to create>\n");
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  num_nitrogen = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  // Convert string from ascii command line argument to integer number
  num_oxygen = dstrtol(argv[2], NULL, 10); // the "10" means base 10
  // Convert string from ascii command line argument to integer number
  temperature = dstrtol(argv[3], NULL, 10); // the "10" means base 10
  // Printf("Creating %d processes\n", numprocs);

  // Allocate space for a shared memory page, which is exactly 64KB
  // Note that it doesn't matter how much memory we actually need: we 
  // always get 64KB
  if ((h_mem = shmget()) == 0) {
    Printf("ERROR: could not allocate shared memory page in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  // Map shared memory page into this process's memory space
  if ((mc = (molecule_count *)shmat(h_mem)) == NULL) {
    Printf("Could not map the shared page to virtual address in "); Printf(argv[0]); Printf(", exiting..\n");
    Exit();
  }

  // Allocate space for a shared memory page, which is exactly 64KB
  // Note that it doesn't matter how much memory we actually need: we 
  // always get 64KB
  if ((h_mem2 = shmget()) == 0) {
    Printf("ERROR: could not allocate shared memory page 2 in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  // Map shared memory page into this process's memory space
  if ((ls = (lock_struct *)shmat(h_mem2)) == NULL) {
    Printf("Could not map the shared page 2 to virtual address in "); Printf(argv[0]); Printf(", exiting..\n");
    Exit();
  }

  // Put some values in the shared memory, to be read by other processes
  mc->o2 = 0;
  mc->n2 = 0;
  mc->no2 = 0;
  mc->o3 = 0;

  numprocs = 6;

  num_o2 = (num_oxygen/2); 
  num_n2 = (num_nitrogen)/2;
  num_no2 = (temperature <= 120)? (((num_o2/2) >= (num_n2))? num_n2: (num_o2/2)): 0;
  num_o3 = (temperature < 60)? 0: (((temperature >= 60) && (temperature <= 120))? ((num_o2 - (2*num_no2))/3): (num_o2/3));

  ls->num_nitrogen = num_nitrogen; 
  ls->num_oxygen = num_oxygen; 
  ls->num_n2 = num_n2; 
  ls->num_o2 = num_o2;    
  ls->num_no2 = num_no2;   
  ls->num_o3 = num_o3;  
/*
  Printf("Inside krypton: temperature = %d\n", temperature);
  Printf("Inside krypton: ls->num_nitrogen = %d\n", ls->num_nitrogen);
  Printf("Inside krypton: ls->num_oxygen = %d\n", ls->num_oxygen);
  Printf("Inside krypton: ls->num_n2 = %d\n", ls->num_n2);
  Printf("Inside krypton: ls->num_o2 = %d\n", ls->num_o2);
  Printf("Inside krypton: ls->num_no2 = %d\n", ls->num_no2);
  Printf("Inside krypton: ls->num_o3 = %d\n", ls->num_o3);
*/
  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.  To do this, we will initialize
  // the semaphore to (-1) * (number of signals), where "number of signals"
  // should be equal to the number of processes we're spawning - 1.  Once 
  // each of the processes has signaled, the semaphore should be back to
  // zero and the final sem_wait below will return.
  if ((s_procs_completed = sem_create(-(numprocs-1))) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  // Create locks
  if ((ls->n_lock = lock_create()) == SYNC_FAIL) {
    Printf("Bad lock_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->o_lock = lock_create()) == SYNC_FAIL) {
    Printf("Bad lock_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->n2_lock = lock_create()) == SYNC_FAIL) {
    Printf("Bad lock_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->o2_lock = lock_create()) == SYNC_FAIL) {
    Printf("Bad lock_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  // Create cond_var
  if ((ls->two_n_available = cond_create(ls->n_lock)) == SYNC_FAIL) {
    Printf("Bad cond_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->two_o_available = cond_create(ls->o_lock)) == SYNC_FAIL) {
    Printf("Bad cond_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->one_n2_available = cond_create(ls->n2_lock)) == SYNC_FAIL) {
    Printf("Bad cond_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->two_o2_available = cond_create(ls->o2_lock)) == SYNC_FAIL) {
    Printf("Bad cond_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }
  if ((ls->three_o2_available = cond_create(ls->o2_lock)) == SYNC_FAIL) {
    Printf("Bad cond_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  // Setup the command-line arguments for the new process.  We're going to
  // pass the handles to the shared memory page and the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(h_mem, h_mem_str);
  ditoa(h_mem2, h_mem2_str);
  ditoa(temperature, temperature_str);
  ditoa(s_procs_completed, s_procs_completed_str);

  // Now we can create the processes.  Note that you MUST end your call to
  // process_create with a NULL argument so that the operating system
  // knows how many arguments you are sending.
  process_create(NITROGEN_INJECT_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, temperature_str, h_mem2_str, NULL);
  process_create(OXYGEN_INJECT_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, temperature_str, h_mem2_str, NULL);
  process_create(NITROGEN_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, temperature_str, h_mem2_str, NULL);
  process_create(OXYGEN_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, temperature_str, h_mem2_str, NULL);
  process_create(NO2_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, temperature_str, h_mem2_str, NULL);
  process_create(OZONE_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, temperature_str, h_mem2_str, NULL);

  // And finally, wait until all spawned processes have finished.
  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
    Exit();
  }
  // Printf("All other processes completed, exiting main process.\n");
}
