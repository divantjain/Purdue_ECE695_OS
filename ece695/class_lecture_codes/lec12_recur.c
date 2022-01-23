#include <stdio.h>
#include <stdlib.h>
 
int global;

static void Loop(int i)
{
  static char *startAddr;
  char local;
  printf("&startAddr=%p, &local=%p &i=%p\n", &startAddr, &local, &i);
  sleep (1);
  Loop(i);
}
 
int main(int argc, char *argv[])
{
  int i, *ptr1, *ptr2;
  printf("&main() = %p, &Loop() = %p\n", &main, &Loop);

  printf("&global = %p \n&ptr1 = %p, &ptr2 = %p\n", &global, &ptr1, &ptr2);

  ptr1 = (int *) malloc(16);
  ptr2 = (int *) malloc(16);
  printf("ptr1 = %p, ptr2 = %p\n", ptr1, ptr2);

  Loop(i);
}
