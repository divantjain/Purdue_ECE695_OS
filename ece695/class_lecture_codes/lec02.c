#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

long myval;

int main(int argc, char *argv[])
{
  myval = atol(argv[1]);
  while (1){
    sleep (1);
    printf("myval = %d, loc = 0x%lx\n",
           myval, (long) &myval);
  }
  //  exit(0);
}

