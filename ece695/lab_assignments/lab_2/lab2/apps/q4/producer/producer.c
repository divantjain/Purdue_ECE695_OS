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
  cond_t s_fullslots;        // Cond used to wait for not full
  cond_t s_emptyslots;        // Cond used to wait for not empty
  lock_t buffer_access_lock;        // Lock used to access circular buffer
  int i;
  char* buffer;
  char main_string[]="Hello World";
  int strlen_of_main_string = 11;

  if (argc != 6) { 
    Printf("Usage: "); Printf(argv[0]); Printf(" <handle_to_shared_memory_page> <handle_to_page_mapped_semaphore>\n"); 
    Exit();
  } 

  // Convert the command-line strings into integers for use as handles
  h_mem = dstrtol(argv[1], NULL, 10); // The "10" means base 10
  s_procs_completed = dstrtol(argv[2], NULL, 10);
  buffer_access_lock = dstrtol(argv[3], NULL, 10);
  s_fullslots = dstrtol(argv[4], NULL, 10);
  s_emptyslots = dstrtol(argv[5], NULL, 10);

  // Map shared memory page into this process's memory space
  if ((cb = (circular_buffer *)shmat(h_mem)) == NULL) {
    Printf("Could not map the virtual address to the memory in "); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
  buffer = cb->buffer;

  for(i=0;i<strlen_of_main_string;i++){ // TODO: see alt for strlen
    lock_acquire(buffer_access_lock);
    if(check_for_buffer_full(cb)){
      cond_wait(s_emptyslots); // s_emptyslots: NOT FULL?
    }
    add_char_to_buffer(buffer,cb,main_string[i]);
    cond_signal(s_fullslots); // s_fullslots: NOT EMPTY?
    lock_release(buffer_access_lock);
    Printf("Producer %d inserted: %c\n",Getpid(),main_string[i]);
  }
 
  // Signal the semaphore to tell the original process that we're done
  if(sem_signal(s_procs_completed) != SYNC_SUCCESS) {
    Printf("Bad semaphore s_procs_completed (%d) in ", s_procs_completed); Printf(argv[0]); Printf(", exiting...\n");
    Exit();
  }
}
