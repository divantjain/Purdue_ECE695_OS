// A C program to demonstrate buffer overflow 
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
  
int main(int argc, char *argv[]) 
{ 
	int k;  
       // Reserve 5 byte of buffer plus the terminating NULL. 
       // should allocate 8 bytes = 2 double words, 
       char buffer[5];  // If more than 8 characters input? 
       int i;
 
       // a prompt how to execute the program... 
       if (argc < 2) 
       { 
              printf("strcpy() NOT executed....\n"); 
              printf("Syntax: %s <characters>\n", argv[0]); 
              exit(0); 
       } 
 
	printf(">>>> BEFORE below k: addr = %p, value=%d\n", (int *)(&k+1), * (int *)(&k+1));      
       // copy the user input to mybuffer, without any 
       strcpy(buffer, argv[1]); 

       printf("&k=%p buffer=%p &i=%p buffer content= %s\n", &k, buffer, &i, buffer); 
  
       printf("strcpy() executed...\n"); 

       printf(">>>> AFTER below k: addr = %p, value=%d\n", (int *)(&k+1), * (int *)(&k+1)); 
       return 0; 
} 