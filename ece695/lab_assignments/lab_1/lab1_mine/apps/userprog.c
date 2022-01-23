#include "usertraps.h"

void main (int x)
{
  unsigned int pid;

  Printf("Hello World!\n");
  pid = Getpid();
  Printf("Got the process id of Current Process\n");
  Printf("The pid is %u.\n", pid);
  Printf("Program Finished! You can press CTRL-C to exit the simulator\n");
  while(1); // Use CTRL-C to exit the simulator
}
