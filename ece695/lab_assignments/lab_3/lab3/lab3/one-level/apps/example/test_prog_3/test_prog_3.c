#include "usertraps.h"
#include "misc.h"

#define MEM_PAGESIZE (0x1000)

int *dummy_recursive_function (int *local_stack_var_ptr)
{
  int stack_frame;
  int stack_size;
  int local_stack_var = 0;

  local_stack_var = *local_stack_var_ptr;

  local_stack_var = local_stack_var + 1;

  stack_frame = (int)local_stack_var_ptr - (int)(&(local_stack_var));

  stack_size = (MEM_PAGESIZE / stack_frame);
  
  //Printf("test_prog_3 (%d): Address of local_stack_var_ptr = %d\n", getpid(), (int)local_stack_var_ptr);
  //Printf("test_prog_3 (%d): Address of local_stack_var = %d\n", getpid(), (int)&local_stack_var);
  //Printf("test_prog_3 (%d): stack_frame = %d\n", getpid(), stack_frame);
  //Printf("test_prog_3 (%d): stack_size = %d\n", getpid(), stack_size);
  //Printf("test_prog_3 (%d): local_stack_var = %d\n", getpid(), local_stack_var);

  if(local_stack_var > stack_size)
    return local_stack_var;
  else 
    return dummy_recursive_function(&local_stack_var);
}

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
  Printf("test_prog_3 (%d): Starting Test!\n", getpid());
  
  Printf("test_prog_3 (%d): Cause the user function call stack to grow larger than 1 page. Program should enter Page Fault Handler after this, and should return gracefully.\n", getpid());
  value = 0;
  ptr = dummy_recursive_function(&value);
  Printf("test_prog_3 (%d): SUCCESS: Program returned gracefully from the Page Fault Handler after stack overflow\n", getpid());

  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("test_prog_3 (%d): Bad semaphore s_procs_completed (%d)!\n", getpid(), s_procs_completed);
    Exit();
  }

  Printf("test_prog_3 (%d): Done!\n", getpid());
}
