#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

void main (int argc, char *argv[])
{
  int numprocs = 0;               // Used to store number of processes to create
  int i;                          // Loop index variable
  circular_buffer *mc;               // Used to get address of shared memory page
  uint32 h_mem;                   // Used to hold handle to shared memory page
  sem_t s_procs_completed;        // Semaphore used to wait until all spawned processes have completed
  sem_t s_fullslots;        // Semaphore used to wait for not full
  sem_t s_emptyslots;        // Semaphore used to wait for not empty
  lock_t buffer_access_lock;        // Lock used to access circular buffer
  char h_mem_str[10];             // Used as command-line argument to pass mem_handle to new processes
  char s_procs_completed_str[10]; // Used as command-line argument to pass page_mapped handle to new processes
  char buffer_access_lock_str[10]; // Used as command-line argument to pass buffer_access_lock handle to new processes
  char s_fullslots_str[10]; // Used as command-line argument to pass buffer_access_lock handle to new processes
  char s_emptyslots_str[10]; // Used as command-line argument to pass buffer_access_lock handle to new processes

  if (argc != 2) {
    Printf("Usage: "); Printf(argv[0]); Printf(" <number of processes to create>\n");
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  numprocs = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  // Printf("Creating %d processes\n", numprocs);

  // Allocate space for a shared memory page, which is exactly 64KB
  // Note that it doesn't matter how much memory we actually need: we 
  // always get 64KB
  if ((h_mem = shmget()) == 0) {
    Printf("ERROR: could not allocate shared memory page in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }

  // Map shared memory page into this process's memory space
  if ((mc = (circular_buffer *)shmat(h_mem)) == NULL) {
    Printf("Could not map the shared page to virtual address in "); Printf(argv[0]); Printf(", exiting..\n");
    Exit();
  }

  // Put some values in the shared memory, to be read by other processes
  mc->head = 0;
  mc->tail = 0;
  mc->buffer_size = BUFFERSIZE; // Change buffer[12] in spawn.h also

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
  // Create lock to access the circular buffer
  if ((buffer_access_lock = lock_create()) == SYNC_FAIL) {
    Printf("Bad lock_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  if ((s_fullslots = sem_create(0)) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  if ((s_emptyslots = sem_create((BUFFERSIZE-1))) == SYNC_FAIL) {
    Printf("Bad sem_create in "); Printf(argv[0]); Printf("\n");
    Exit();
  }

  // Setup the command-line arguments for the new process.  We're going to
  // pass the handles to the shared memory page and the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(h_mem, h_mem_str);
  ditoa(s_procs_completed, s_procs_completed_str);
  ditoa(buffer_access_lock, buffer_access_lock_str);
  ditoa(s_fullslots, s_fullslots_str);
  ditoa(s_emptyslots, s_emptyslots_str);

  // Now we can create the processes.  Note that you MUST end your call to
  // process_create with a NULL argument so that the operating system
  // knows how many arguments you are sending.
  for(i=0; i<numprocs; i++) {
    // process_create(FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, buffer_access_lock_str, NULL);
    process_create(PRODUCER_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, buffer_access_lock_str, 
                                           s_fullslots_str, s_emptyslots_str, NULL);
    process_create(CONSUMER_FILENAME_TO_RUN, h_mem_str, s_procs_completed_str, buffer_access_lock_str,
                                           s_fullslots_str, s_emptyslots_str, NULL);
    // Printf("Process %d created\n", i);
  }

  // And finally, wait until all spawned processes have finished.
  if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf("\n");
    Exit();
  }
  // Printf("All other processes completed, exiting main process.\n");
}
