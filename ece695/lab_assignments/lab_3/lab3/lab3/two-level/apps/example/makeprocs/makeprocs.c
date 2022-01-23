#include "usertraps.h"
#include "misc.h"

#define HELLO_WORLD "hello_world.dlx.obj"
#define TEST_PROG_1 "test_prog_1.dlx.obj"
#define TEST_PROG_2 "test_prog_2.dlx.obj"
#define TEST_PROG_3 "test_prog_3.dlx.obj"
#define TEST_PROG_4 "test_prog_4.dlx.obj"
#define TEST_PROG_5 "test_prog_5.dlx.obj"
#define TEST_PROG_6 "test_prog_6.dlx.obj"

void main (int argc, char *argv[])
{
  int proc_type = 0;                   // Used to store number of processes to create
  int num_procs = 0;                   // Used to store number of processes to create
  int i;                               // Loop index variable
  sem_t s_procs_completed;             // Semaphore used to wait until all spawned processes have completed
  char s_procs_completed_str[10];      // Used as command-line argument to pass page_mapped handle to new processes
  int spawned_proc_id;
  char spawned_proc_id_str[10];

  if (argc != 3) {
    //Printf("Usage: %s <number of hello world processes to create>\n", argv[0]);
    Printf("Usage: %s <which user program to run> <number of user program processes to create>\n", argv[0]);
    Exit();
  }

  // Convert string from ascii command line argument to integer number
  proc_type = dstrtol(argv[1], NULL, 10); // the "10" means base 10
  switch (proc_type) {
    case 0 : Printf("makeprocs (%d): Creating %d hello world process\n", getpid(), proc_type); break;
    case 1 : Printf("makeprocs (%d): Creating %d test_prog_1 process\n", getpid(), proc_type); break;
    case 2 : Printf("makeprocs (%d): Creating %d test_prog_2 process\n", getpid(), proc_type); break;
    case 3 : Printf("makeprocs (%d): Creating %d test_prog_3 process\n", getpid(), proc_type); break;
    case 4 : Printf("makeprocs (%d): Creating %d test_prog_4 process\n", getpid(), proc_type); break;
    case 5 : Printf("makeprocs (%d): Creating %d test_prog_5 process\n", getpid(), proc_type); break;
    case 6 : Printf("makeprocs (%d): Creating %d test_prog_6 process\n", getpid(), proc_type); break;
  }
  num_procs = dstrtol(argv[2], NULL, 10); // the "10" means base 10
  Printf("makeprocs (%d): Creating %d processes\n", getpid(), num_procs);

  // Create semaphore to not exit this process until all other processes 
  // have signalled that they are complete.
  if ((s_procs_completed = sem_create(0)) == SYNC_FAIL) {
    Printf("makeprocs (%d): Bad sem_create\n", getpid());
    Exit();
  }

  // Setup the command-line arguments for the new processes.  We're going to
  // pass the handles to the semaphore as strings
  // on the command line, so we must first convert them from ints to strings.
  ditoa(s_procs_completed, s_procs_completed_str);

  // Create Hello World processes
  Printf("-------------------------------------------------------------------------------------\n");
  if(proc_type != 5)
  {
    Printf("makeprocs (%d): Creating %d proceses in a row, but only one runs at a time\n", getpid(), num_procs);
    for(i=0; i<num_procs; i++) {
      switch (proc_type) {
        case 0 : 
          Printf("makeprocs (%d): Creating hello world #%d\n", getpid(), i);
          process_create(HELLO_WORLD, s_procs_completed_str, NULL);
          if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
            Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
            Exit();
          }
          break;
        case 1 : 
          Printf("makeprocs (%d): Creating test_prog_1 #%d\n", getpid(), i);
          process_create(TEST_PROG_1, s_procs_completed_str, NULL);
          if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
            Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
            Exit();
          }
          break;
        case 2 : 
          Printf("makeprocs (%d): Creating test_prog_2 #%d\n", getpid(), i);
          process_create(TEST_PROG_2, s_procs_completed_str, NULL);
          if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
            Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
            Exit();
          }
          break;
        case 3 : 
          Printf("makeprocs (%d): Creating test_prog_3 #%d\n", getpid(), i);
          process_create(TEST_PROG_3, s_procs_completed_str, NULL);
          if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
            Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
            Exit();
          }
          break;
        case 4 : 
          Printf("makeprocs (%d): Creating test_prog_4 #%d\n", getpid(), i);
          process_create(TEST_PROG_4, s_procs_completed_str, NULL);
          if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
            Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
            Exit();
          }
          break;
        case 6 : 
          Printf("makeprocs (%d): Creating test_prog_6 #%d\n", getpid(), i);
          process_create(TEST_PROG_6, s_procs_completed_str, NULL);
          if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
            Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
            Exit();
          }
          break;
      }
    }
  }
  else
  {
    Printf("makeprocs (%d): Creating %d proceses in a row, but all runs Simultaneously.\n", getpid(), num_procs);
    for(i=0; i<num_procs; i++) {
      //Printf("makeprocs (%d): Creating test_prog_5 #%d\n", getpid(), i);
      spawned_proc_id = i;
      ditoa(spawned_proc_id, spawned_proc_id_str);      
      process_create(TEST_PROG_5, s_procs_completed_str, spawned_proc_id_str, NULL);
    }
    for(i=0; i<num_procs; i++) {
      //Printf("makeprocs (%d): Waiting for Ending test_prog_5 #%d\n", getpid(), i);
      if (sem_wait(s_procs_completed) != SYNC_SUCCESS) {
        Printf("Bad semaphore s_procs_completed (%d) in %s\n", s_procs_completed, argv[0]);
        Exit();
      }
    }
  } 


  Printf("-------------------------------------------------------------------------------------\n");
  Printf("makeprocs (%d): All other processes completed, exiting main process.\n", getpid());

}
