//
//	memory.c
//
//	Routines for dealing with memory management.

//static char rcsid[] = "$Id: memory.c,v 1.1 2000/09/20 01:50:19 elm Exp elm $";

#include "ostraps.h"
#include "dlxos.h"
#include "process.h"
#include "memory.h"
#include "queue.h"

// num_pages = size_of_memory / size_of_one_page
static uint32 freemap[MEM_NUM_PHY_PAGES_FREEMAP];
static uint32 pagestart;
static int nfreepages;
static int freemapmax;

//----------------------------------------------------------------------
//
//	This silliness is required because the compiler believes that
//	it can invert a number by subtracting it from zero and subtracting
//	an additional 1.  This works unless you try to negate 0x80000000,
//	which causes an overflow when subtracted from 0.  Simply
//	trying to do an XOR with 0xffffffff results in the same code
//	being emitted.
//
//----------------------------------------------------------------------
static int negativeone = 0xFFFFFFFF;
static inline uint32 invert (uint32 n) {
  return (n ^ negativeone);
}

//----------------------------------------------------------------------
//
//	MemoryGetSize
//
//	Return the total size of memory in the simulator.  This is
//	available by reading a special location.
//
//----------------------------------------------------------------------
int MemoryGetSize() {
  return (*((int *)DLX_MEMSIZE_ADDRESS));
}


//----------------------------------------------------------------------
//
//	MemoryModuleInit
//
//	Initialize the memory module of the operating system.
//      Basically just need to setup the freemap for pages, and mark
//      the ones in use by the operating system as "VALID", and mark
//      all the rest as not in use.
//
//----------------------------------------------------------------------
void MemoryModuleInit() 
{
  int i, j;
  int num_os_page = (lastosaddress/MEM_PAGESIZE + 1);
    
  nfreepages = (MEM_NUM_PHY_PAGES - num_os_page);

  //Initializing all other pages to 0 i.e. they are ready to be used for the user programs 
  for (i=0; i<MEM_NUM_PHY_PAGES_FREEMAP; i++){
    freemap[i] = 0x00000000;
  }

  //Initializing pages corresponding to OS address to 1 i.e. they are in-use and cannot be used for the user programs 
  for (i=0; i<num_os_page/32; i++){
    freemap[i] = 0xffffffff;
  }  
  for (j=0; j<(num_os_page%32); j++){
    freemap[i] = freemap[i] | (0x000000001 << j);
  }
    
  //dbprintf('m',"MemoryModuleInit: lastosaddress: %d num_os_page: %d freemap[i]: %d nfreepages: %d\n", lastosaddress, num_os_page, freemap[i], nfreepages);
}


//----------------------------------------------------------------------
//
// MemoryTranslateUserToSystem
//
//	Translate a user address (in the process referenced by pcb)
//	into an OS (physical) address.  Return the physical address.
//
//----------------------------------------------------------------------
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr) 
{ 
  uint32 virtual_page_num;
  uint32 offset;
  uint32 pte;
  uint32 phy_page_num;
  uint32 phy_addr;

  virtual_page_num = (addr / MEM_PAGESIZE);
  offset = (addr % MEM_PAGESIZE);
  pte = pcb->pagetable[virtual_page_num];
  phy_page_num = (pte & MEM_PTE2PADDR_MASK);
  phy_page_num = ((phy_page_num >> MEM_L1FIELD_FIRST_BITNUM) << MEM_L1FIELD_FIRST_BITNUM); // Zero-ing out the offset bits of PTE
  phy_addr = (phy_page_num + offset);

  if (virtual_page_num > MEM_L1_PTSIZE) {
    printf("ERROR: Virtual Address is larger than Max Virtual address. In MemoryTranslateUserToSystem!\n");
    return (0);
  }

  return phy_addr;
}


//----------------------------------------------------------------------
//
//	MemoryMoveBetweenSpaces
//
//	Copy data between user and system spaces.  This is done page by
//	page by:
//	* Translating the user address into system space.
//	* Copying all of the data in that page
//	* Repeating until all of the data is copied.
//	A positive direction means the copy goes from system to user
//	space; negative direction means the copy goes from user to system
//	space.
//
//	This routine returns the number of bytes copied.  Note that this
//	may be less than the number requested if there were unmapped pages
//	in the user range.  If this happens, the copy stops at the
//	first unmapped address.
//
//----------------------------------------------------------------------
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir) {
  unsigned char *curUser;         // Holds current physical address representing user-space virtual address
  int		bytesCopied = 0;  // Running counter
  int		bytesToCopy;      // Used to compute number of bytes left in page to be copied

  while (n > 0) {
    // Translate current user page to system address.  If this fails, return
    // the number of bytes copied so far.
    curUser = (unsigned char *)MemoryTranslateUserToSystem (pcb, (uint32)user);

    // If we could not translate address, exit now
    if (curUser == (unsigned char *)0) break;

    // Calculate the number of bytes to copy this time.  If we have more bytes
    // to copy than there are left in the current page, we'll have to just copy to the
    // end of the page and then go through the loop again with the next page.
    // In other words, "bytesToCopy" is the minimum of the bytes left on this page 
    // and the total number of bytes left to copy ("n").

    // First, compute number of bytes left in this page.  This is just
    // the total size of a page minus the current offset part of the physical
    // address.  MEM_PAGESIZE should be the size (in bytes) of 1 page of memory.
    // MEM_ADDRESS_OFFSET_MASK should be the bit mask required to get just the
    // "offset" portion of an address.
    bytesToCopy = MEM_PAGESIZE - ((uint32)curUser & MEM_ADDRESS_OFFSET_MASK);
    
    // Now find minimum of bytes in this page vs. total bytes left to copy
    if (bytesToCopy > n) {
      bytesToCopy = n;
    }

    // Perform the copy.
    if (dir >= 0) {
      bcopy (system, curUser, bytesToCopy);
    } else {
      bcopy (curUser, system, bytesToCopy);
    }

    // Keep track of bytes copied and adjust addresses appropriately.
    n -= bytesToCopy;           // Total number of bytes left to copy
    bytesCopied += bytesToCopy; // Total number of bytes copied thus far
    system += bytesToCopy;      // Current address in system space to copy next bytes from/into
    user += bytesToCopy;        // Current virtual address in user space to copy next bytes from/into
  }
  return (bytesCopied);
}

//----------------------------------------------------------------------
//
//	These two routines copy data between user and system spaces.
//	They call a common routine to do the copying; the only difference
//	between the calls is the actual call to do the copying.  Everything
//	else is identical.
//
//----------------------------------------------------------------------
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, from, to, n, 1));
}

int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from,unsigned char *to, int n) {
  return (MemoryMoveBetweenSpaces (pcb, to, from, n, -1));
}

//---------------------------------------------------------------------
// MemoryPageFaultHandler is called in traps.c whenever a page fault 
// (better known as a "seg fault" occurs.  If the address that was
// being accessed is on the stack, we need to allocate a new page 
// for the stack.  If it is not on the stack, then this is a legitimate
// seg fault and we should kill the process.  Returns MEM_SUCCESS
// on success, and kills the current process on failure.  Note that
// fault_address is the beginning of the page of the virtual address that 
// caused the page fault, i.e. it is the vaddr with the offset zero-ed
// out.
//
// Note: The existing code is incomplete and only for reference. 
// Feel free to edit.
//---------------------------------------------------------------------
int MemoryPageFaultHandler(PCB *pcb) 
{
  uint32 faulty_virtual_page;
  uint32 faulty_virtual_page_num;
  uint32 user_stack_pointer;
  uint32 new_page;
  
  faulty_virtual_page = pcb->currentSavedFrame[PROCESS_STACK_FAULT];
  faulty_virtual_page_num = faulty_virtual_page / MEM_PAGESIZE;
  
  user_stack_pointer = pcb->currentSavedFrame[PROCESS_STACK_USER_STACKPOINTER];  
  user_stack_pointer = (user_stack_pointer >> MEM_L1FIELD_FIRST_BITNUM) << MEM_L1FIELD_FIRST_BITNUM; //Zero-ing the offset field

  dbprintf('m',"MemoryPageFaultHandler: Entered\n");
  dbprintf('m',"MemoryPageFaultHandler: faulty_virtual_page: 0x%08x\n", faulty_virtual_page);
  dbprintf('m',"MemoryPageFaultHandler: user_stack_pointer: 0x%08x\n", user_stack_pointer);

  if(faulty_virtual_page >= (user_stack_pointer - 0x8)) {
    dbprintf('m',"MemoryPageFaultHandler: Its a legitimate Page Fault, allocating a new page for the User Stack\n");
    // Allocating 1 more pages for User Stack
    pcb->npages = pcb->npages + 1;
    new_page = MemoryAllocPage();
    if (new_page == 0) {
      printf ("FATAL: couldn't allocate memory for User Stack - no free pages! Inside MemoryPageFaultHandler\n");
      exitsim ();	// NEVER RETURNS!
    }
    pcb->pagetable[faulty_virtual_page_num] = MemorySetupPte(new_page);
    dbprintf('m',"MemoryPageFaultHandler: Leaving. Allocated new physical page:%d\n", new_page);
    return MEM_SUCCESS;
  }
  else {
    dbprintf('m',"MemoryPageFaultHandler: Trying to access an illegal location. Killing the process\n");
    dbprintf('m',"MemoryPageFaultHandler: Leaving\n");
    ProcessKill();
    return MEM_FAIL;
  }
}


//---------------------------------------------------------------------
// You may need to implement the following functions and access them from process.c
// Feel free to edit/remove them
//---------------------------------------------------------------------

//----------------------------------------------------------------------
//
//	MemoryAllocPage
//
//	Allocate a page of memory.
//
//----------------------------------------------------------------------
uint32 MemoryAllocPage(void) 
{
  int i, j; 
  uint32 free_page_num;
  int hit;

  if (nfreepages == 0) {
    dbprintf('m',"MemoryAllocPage: No Free Pages, nfreepages = %d\n", nfreepages);
    return 0;
  }
  
  for (i=0, hit=0; i<MEM_NUM_PHY_PAGES_FREEMAP; i++) {
    for (j=0; j<32; j++) {
      if((freemap[i] & (0x00000001 << j)) == 0) {
        freemap[i] = (freemap[i] | (0x00000001 << j));
	hit = 1;
      }
      if(hit == 1)
        break;
    }
    if(hit == 1)
      break;
  }
  
  free_page_num = i*32 + j;

  nfreepages = nfreepages - 1; 
  
  //dbprintf('m',"MemoryAllocPage: Returing Free Page Number: %d. Updated nfreepages: %d, i=%d, j=%d\n", free_page_num, nfreepages, i , j);

  return free_page_num;
}


//----------------------------------------------------------------------
//
// 	MemorySetupPte
//
//	Set up a PTE given a page number.
//
//----------------------------------------------------------------------
uint32 MemorySetupPte(uint32 page) 
{
  //dbprintf('m',"MemorySetupPte: ((page * MEM_PAGESIZE) | MEM_PTE_VALID) = %d\n", ((page * MEM_PAGESIZE) | MEM_PTE_VALID));
  return ((page * MEM_PAGESIZE) | MEM_PTE_VALID);
}


//----------------------------------------------------------------------
//
//	MemoryFreePage
//
//	Free a page of memory.
//
//----------------------------------------------------------------------
void MemoryFreePage(uint32 page) 
{
  uint32 freemap_array_element_num;
  uint32 bitnum;
  uint32 bitmask;

  freemap_array_element_num = (page / 32);
  bitnum = (page % 32);
  bitmask = (1 << bitnum);
  bitmask = invert(bitmask);
  
  //dbprintf('m',"MemoryFreePage: page: %d, freemap_array_element_num: %d, bitnum: %d, bitmask: 0x%08x\n", page, freemap_array_element_num, bitnum, bitmask);

  freemap[freemap_array_element_num] = freemap[freemap_array_element_num] & (bitmask);
  nfreepages = nfreepages + 1;
}


//----------------------------------------------------------------------
//
// 	MemoryFreePte
//
//	Free a page given its PTE.
//
//----------------------------------------------------------------------
void
MemoryFreePte (uint32 pte)
{
  MemoryFreePage((pte & MEM_PTE2PADDR_MASK) / MEM_PAGESIZE);
}


int malloc (PCB *pcb, int memsize) {
  return -1;
}

int mfree (PCB *pcb, void *ptr) {
  return -1;
}

