#ifndef __USERPROG__
#define __USERPROG__

typedef struct missile_code {
  int numprocs;
  char really_important_char;
} missile_code;

typedef struct circular_buffer {
  int head, tail;
  int buffer_size;
  char buffer[BUFFERSIZE]; // For fitting Hello World
} circular_buffer;

#define FILENAME_TO_RUN "spawn_me.dlx.obj"
#define PRODUCER_FILENAME_TO_RUN "producer.dlx.obj"
#define CONSUMER_FILENAME_TO_RUN "consumer.dlx.obj"

#define TRUE 1
#define FALSE 0

#endif
