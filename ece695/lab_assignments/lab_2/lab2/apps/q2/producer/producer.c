#include "lab2-api.h"
#include "usertraps.h"
#include "misc.h"

#include "spawn.h"

int check_for_buffer_full(circular_buffer *cb){
  if (((cb->head + 1) % BUFFERSIZE) == cb->tail) {
      // buffer is full
       return TRUE;
    } else {
      // buffer is not full
       return FALSE;
    }
}
int add_char_to_buffer(char* buffer, circular_buffer *cb, char c){
    buffer[cb->head] = c;
    if(cb->head == cb->buffer_size -1){
      cb->head = 0;
    }
    else
      cb->head += 1;
}

void main (int argc, char *argv[])
{
  circular_buffer *cb;        // Used to access missile codes in shared memory page
  uint32 h_mem;            // Handle to the shared memory page
  sem_t s_procs_completed; // Semaphore to signal the original process that we're done
  lock_t buffer_access_lock;        // Lock used to access circular buffer
  int i;
  char* buffer;
  char main_string[]="Hello World";
  int strlen_of_main_string = 11;

  if (argc != 4) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);
  buffer_access_lock = dstrtol(argv[3], NULL, 10);

  // Map shared memory page into this process's memory space
  if ((cb = (circular_buffer *)shmat(h_mem)) == NULL) {
    Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  buffer = cb->buffer;

  for(i=0;i<strlen_of_main_string;i++){ // TODO: see alt for strlen
    while(lock_acquire(buffer_access_lock) == SYNC_FAIL);
    // Printf("Prod Got the lock");
    // Check for buffer full
    if(check_for_buffer_full(cb)){
      // Release lock and wait for consumer
      lock_release(buffer_access_lock);
      i--;
      continue;
    }
    add_char_to_buffer(buffer,cb,main_string[i]);
    Printf("Producer %d inserted: %c\n",Getpid(),main_string[i]);
    // Printf("\nLock releasing ");
    lock_release(buffer_access_lock);
  }
 
  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
