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



//----------------------------------------------------------------------
//
// 	HEAP MANAGEMENT
//
//----------------------------------------------------------------------
int CalculatePower(int base, int power)
{
  int i;
  int result = 1;

  for (i=1; i<=power; i++)
  {
    result = result * base;
  }
  
  return result;
}

void PrintHeapNode(heap_node *node)
{
  dbprintf('m', "PrintHeapNode: node->order: %d\n", node->order);
  dbprintf('m', "PrintHeapNode: node->addr: %d\n", node->addr);
  dbprintf('m', "PrintHeapNode: node->size: %d\n", node->size);
  dbprintf('m', "PrintHeapNode: node->created: %d\n", node->created);
  dbprintf('m', "PrintHeapNode: node->occupied: %d\n", node->occupied);
  dbprintf('m', "PrintHeapNode: node->parent: %d\n", (uint32)node->parent);
  dbprintf('m', "PrintHeapNode: node->left: %d\n", (uint32)node->left);
  dbprintf('m', "PrintHeapNode: node->right: %d\n", (uint32)node->right);
  dbprintf('m', "PrintHeapNode: node->buddy: %d\n", (uint32)node->buddy);
}

void AssignHeapNode (heap_node *node, int order, uint32 addr, int size, int created, int occupied, int splitted, heap_node *parent, heap_node *left, heap_node *right, heap_node *buddy)
{
  node->order = order;
  node->addr = addr;
  node->size = size;
  node->created = created;
  node->occupied = occupied;
  node->splitted = splitted;
  node->parent = parent;
  node->left = left;
  node->right = right;
  node->buddy = buddy;
}

void InitializeHeapTree (PCB *pcb)
{
  int i, j;
  int current_index;
  int order;
  uint32 addr;
  int size;
  int created;
  int occupied;
  int splitted;
  heap_node *parent;
  heap_node *left;
  heap_node *right;
  heap_node *buddy;

  //dbprintf('m', "InitializeHeapTree: pcb: %d\n", (int)pcb);

  for (i=0; i<=MEM_HEAP_MAX_ORDER; i++) 
  {
    for (j=0; j<CalculatePower(2,i); j++)
    {
      current_index = ((CalculatePower(2,i) - 1) + (j));
      order = (MEM_HEAP_MAX_ORDER - i);
      size = (MEM_HEAP_ORDER_0_SIZE * CalculatePower(2, (MEM_HEAP_MAX_ORDER - i)));
      addr = (j * size);
      created = 0;
      occupied = 0;
      splitted = 0;
      if(i==0)
      {
        parent = NULL; 
        buddy = NULL;
      }
      else
      {
        parent = &(pcb->heap_tree[((CalculatePower(2,(i-1)) - 1) + (j/2))]);
        if(j%2 == 0)
        {
          buddy = &(pcb->heap_tree[(CalculatePower(2,i) - 1) + (j+1)]);
        }
        else
        {
          buddy = &(pcb->heap_tree[(CalculatePower(2,i) - 1) + (j-1)]);
        }
      }
      
      if(i==MEM_HEAP_MAX_ORDER)
      {
        left = NULL; 
        right = NULL;
      }
      else
      {
        left = &(pcb->heap_tree[((CalculatePower(2,(i+1)) - 1) + (j*2))]);
        right = &(pcb->heap_tree[((CalculatePower(2,(i+1)) - 1) + (j*2) + (1))]);
      }
      
      AssignHeapNode(&(pcb->heap_tree[current_index]), order, addr, size, created, occupied, splitted, parent, left, right, buddy);
      
      //dbprintf('m', "InitializeHeapTree: i:%d, j:%d, current_index:%d\n", i, j, current_index);
      //dbprintf('m', "InitializeHeapTree: heap_tree[%d].order: %d\n", current_index, pcb->heap_tree[current_index].order);
      //dbprintf('m', "InitializeHeapTree: heap_tree[%d].size: %d\n", current_index, pcb->heap_tree[current_index].size);
      //dbprintf('m', "InitializeHeapTree: heap_tree[%d].addr: %d\n", current_index, pcb->heap_tree[current_index].addr);
    }
  }
}

void FreeHeapTree (PCB *pcb)
{
  int i, j;
  int current_index;
  int order;
  uint32 addr;
  int size;
  int created;
  int occupied;
  int splitted;
  heap_node *parent;
  heap_node *left;
  heap_node *right;
  heap_node *buddy;

  for (i=0; i<=MEM_HEAP_MAX_ORDER; i++) 
  {
    for (j=0; j<CalculatePower(2,i); j++)
    {
      current_index = ((CalculatePower(2,i) - 1) + (j));
      order = pcb->heap_tree[0].order;
      size = pcb->heap_tree[0].size;
      addr = pcb->heap_tree[0].addr;
      created = 0;
      occupied = 0;
      splitted = 0;
      parent = pcb->heap_tree[0].parent;
      left = pcb->heap_tree[0].left;
      right = pcb->heap_tree[0].right;
      buddy = pcb->heap_tree[0].buddy;
      
      AssignHeapNode(&(pcb->heap_tree[current_index]), order, addr, size, created, occupied, splitted, parent, left, right, buddy);
    }
  }
}

int GetRequiredOrder (int memsize)
{
  int i;
  int hit;
  int required_order = -1;

  for (i=0, hit=0; i<=MEM_HEAP_MAX_ORDER; i++)
  {
    if((memsize / (CalculatePower(2, i) * MEM_HEAP_ORDER_0_SIZE)) == 0)
    {
      hit = 1;
      required_order = i;
      break;
    }
  }
  
  if (hit == 0)
  {
    dbprintf('m', "FATAL: Inside GetRequiredOrder: couldn't get the required order for required memsize\n");

    return -1;
  }
  else
  {
    return required_order;
  }
}

void CreteTwoBuddyNodes (heap_node *node)
{
  if (node == node->parent->left)
  {
    node->created = 1;
    printf("Created a left child node (order = %d, addr = %d, size = %d) of parent (order = %d, addr = %d, size = %d)\n", node->order, node->addr, node->size, node->parent->order, node->parent->addr, node->parent->size);
    node->buddy->created = 1;
    printf("Created a right child node (order = %d, addr = %d, size = %d) of parent (order = %d, addr = %d, size = %d)\n", node->buddy->order, node->buddy->addr, node->buddy->size, node->parent->order, node->parent->addr, node->parent->size);
    node->parent->splitted = 1;
  }
  else
  {
    node->buddy->created = 1;
    printf("Created a left child node (order = %d, addr = %d, size = %d) of parent (order = %d, addr = %d, size = %d)\n", node->buddy->order, node->buddy->addr, node->buddy->size, node->parent->order, node->parent->addr, node->parent->size);
    node->created = 1;
    printf("Created a right child node (order = %d, addr = %d, size = %d) of parent (order = %d, addr = %d, size = %d)\n", node->order, node->addr, node->size, node->parent->order, node->parent->addr, node->parent->size);
    node->parent->splitted = 1;
  }
}

void CreateNode(heap_node *node)
{
  if(node->parent->created == 1)
  {
    CreteTwoBuddyNodes(node);
  }
  else
  {
    CreateNode(node->parent);
    CreteTwoBuddyNodes(node);
  }
}

uint32 SearchAndAllocateNodeForOrder (PCB *pcb, int order_required, int memsize)
{
  int i, j, order;
  uint32 heap_addr_offset = -1;

  //for (i=0, order=MEM_HEAP_MAX_ORDER; i<=MEM_HEAP_TREE_MAX_ELEM, order>=0; order--) // warning: left-hand operand of comma expression has no effect
  for (i=0, order=MEM_HEAP_MAX_ORDER; order>=0; order--)
  {
    if(pcb->heap_tree[i].created == 1)
    {
      if(pcb->heap_tree[i].order == order_required)
      {
        if((pcb->heap_tree[i].occupied == 0) && (pcb->heap_tree[i].splitted == 0))
        {
          printf("Allocated the block: order = %d, addr = %d, requested mem size = %d, block size = %d\n", pcb->heap_tree[i].order, pcb->heap_tree[i].addr, memsize, pcb->heap_tree[i].size);
          pcb->heap_tree[i].occupied = 1;
          return pcb->heap_tree[i].addr;
        }
        else if ((pcb->heap_tree[i].buddy->occupied == 0) && (pcb->heap_tree[i].buddy->splitted == 0)) // Checking for its buddy
        {
          printf("Allocated the block: order = %d, addr = %d, requested mem size = %d, block size = %d\n", pcb->heap_tree[i].buddy->order, pcb->heap_tree[i].buddy->addr, memsize, pcb->heap_tree[i].buddy->size);
          pcb->heap_tree[i].buddy->occupied = 1;
          return pcb->heap_tree[i].buddy->addr;
        }
        else //Searching all the Nodes of same order
        {
          //dbprintf('m', "Inside SearchAndAllocateNodeForOrder: Searching all the Nodes of same order: i: %d, order: %d, j: %d.\n", i, order, j);
          for(j=i+2; j<(CalculatePower(2,((MEM_HEAP_MAX_ORDER - order)+1)) - 1); j++)
          {
            //dbprintf('m', "Inside SearchAndAllocateNodeForOrder: Searching all the Nodes of same order: i: %d, order: %d, j: %d.\n", i, order, j);
            if(pcb->heap_tree[j].created == 1)
            {
              //dbprintf('m', "Inside SearchAndAllocateNodeForOrder: Searching all the Nodes of same order: if.\n");
              if ((pcb->heap_tree[j].occupied == 0) && (pcb->heap_tree[j].splitted == 0))
              {
                printf("Allocated the block: order = %d, addr = %d, requested mem size = %d, block size = %d\n", pcb->heap_tree[j].order, pcb->heap_tree[j].addr, memsize, pcb->heap_tree[j].size);
                pcb->heap_tree[j].occupied = 1;
                return pcb->heap_tree[j].addr;
              }
            }
            else
            {
              //dbprintf('m', "Inside SearchAndAllocateNodeForOrder: Searching all the Nodes of same order: else.\n");
              CreateNode(&(pcb->heap_tree[j]));
              printf("Allocated the block: order = %d, addr = %d, requested mem size = %d, block size = %d\n", pcb->heap_tree[j].order, pcb->heap_tree[j].addr, memsize, pcb->heap_tree[j].size);
              pcb->heap_tree[j].occupied = 1;
              return pcb->heap_tree[j].addr;
            }
          }
        }
      }
      else
      {
        //dbprintf('m', "Inside SearchAndAllocateNodeForOrder: i: %d.\n", i);
        if (pcb->heap_tree[i].left->created == 0) 
        {
          CreteTwoBuddyNodes(pcb->heap_tree[i].left);
        }
        i = (CalculatePower(2,((MEM_HEAP_MAX_ORDER - order)+1)) - 1);
        //dbprintf('m', "Inside SearchAndAllocateNodeForOrder: i: %d.\n", i);
        continue;
      }
    }
    else
    {
      dbprintf('m', "ERROR: Inside SearchAndAllocateNodeForOrder: Trying to test an un-created node.\n");
    }    
  }
  
  return heap_addr_offset;
}
          
uint32 buddy_mem_alloc(PCB *pcb, int memsize)
{
  int order_required;
  uint32 heap_addr_offset;
  
  order_required = GetRequiredOrder (memsize);
  
  heap_addr_offset = SearchAndAllocateNodeForOrder(pcb, order_required, memsize);
  
  return heap_addr_offset;
}

//----------------------------------------------------------------------
//
// 	malloc
//
//	malloc.
//
//----------------------------------------------------------------------
void *malloc (PCB *pcb, int memsize) 
{
  uint32 heap_addr_offset;
  uint32 heap_base_addr;
  uint32 *heap_address_pointer;
  uint32 heap_address;
  uint32 physical_heap_address;

  if(memsize <= 0)
  {
    printf ("FATAL: Inside malloc: couldn't allocate memory for Heap, as requested memory is less than or equal to 0\n");
    return NULL;
  }
  if(memsize > MEM_HEAP_MAX_MEM)
  {
    printf ("FATAL: Inside malloc: couldn't allocate memory for Heap, as requested memory is larger than HEAP available\n");
    return NULL;
  }

  heap_addr_offset = buddy_mem_alloc(pcb, memsize);
  if(heap_addr_offset == -1)
  {
    printf ("FATAL: Inside malloc: couldn't allocate memory for Heap\n");
    return NULL;
  }
  
  heap_base_addr = (MEM_PAGESIZE * 4);
  heap_address = (heap_base_addr + heap_addr_offset);
  physical_heap_address = MemoryTranslateUserToSystem(pcb, heap_address);
  heap_address_pointer = (uint32 *)(heap_address);
  //printf("Created a heap block of size %d bytes: virtual address 0x%08x, physical address 0x%08x.\n", memsize, heap_address, physical_heap_address);
  //printf("Inside malloc: heap_addr_offset: %d, heap_address: 0x%08x, heap_address_pointer: 0x%08x.\n", heap_addr_offset, heap_address, (uint32)heap_address_pointer);

  return heap_address_pointer;
}

//----------------------------------------------------------------------

void CombineBuddies(heap_node *node)
{
  node->created = 0;
  node->buddy->created = 0;
  node->parent->splitted = 0;
  
  printf("Coalesced buddy nodes (order = %d, addr = %d, size = %d) & (order = %d, addr = %d, size = %d)\n", node->order, node->addr, node->size, node->buddy->order, node->buddy->addr, node->buddy->size);
  printf("into the parent node (order = %d, addr = %d, size = %d)\n", node->parent->order, node->parent->addr, node->parent->size);
}

void CheckAndCombineFreeBuddies(heap_node *node)
{
  if (node->buddy->created == 1)
  {
    if (node->buddy->occupied == 0)
    {
      //if ((node->left->created == 0) && (node->right->created == 0) && (node->buddy->left->created == 0) && (node->buddy->right->created == 0))
      if ((node->splitted == 0) && (node->buddy->splitted == 0))
      {
        CombineBuddies(node);
        CheckAndCombineFreeBuddies(node->parent);
      }
      else
      {
        //dbprintf('m', "WARNING: Inside CheckAndCombineFreeBuddies: The buddies have the Child nodes created.\n");
      }    
    }
    else
    {
      //dbprintf('m', "WARNING: Inside CheckAndCombineFreeBuddies: The buddies node is not Free.\n");
    }    
  }
  else
  {
    dbprintf('m', "ERROR: Inside CheckAndCombineFreeBuddies: Found a pair of buddy, where one of them is not created.\n");
  }
}

int SearchForNodeToBeFreed (PCB *pcb, int heap_addr_offset)
{
  int i;
  int num_bytes_freed = -1;

  for (i=0; i<=MEM_HEAP_TREE_MAX_ELEM; i++)
  {
    if(pcb->heap_tree[i].created == 1)
    {
      if(pcb->heap_tree[i].addr == heap_addr_offset)
      {
        if(pcb->heap_tree[i].occupied == 1)
        {
          printf("Freed the block: order = %d, addr = %d, size = %d\n", pcb->heap_tree[i].order, pcb->heap_tree[i].addr, pcb->heap_tree[i].size);
          pcb->heap_tree[i].occupied = 0;
          num_bytes_freed = pcb->heap_tree[i].size;
          CheckAndCombineFreeBuddies(&(pcb->heap_tree[i]));
          return num_bytes_freed;
        }
      }
    }
  }
  
  return -1;
}

int buddy_mem_free(PCB *pcb, int heap_addr_offset)
{
  int num_bytes_freed;

  num_bytes_freed = SearchForNodeToBeFreed(pcb, heap_addr_offset);

  return num_bytes_freed;
}

//----------------------------------------------------------------------
//
// 	mfree
//
//	mfree.
//
//----------------------------------------------------------------------
int mfree (PCB *pcb, void *ptr) 
{
  uint32 heap_addr_offset;
  uint32 heap_base_addr;
  uint32 heap_max_addr;
  uint32 heap_address;
  uint32 physical_heap_address;
  int num_bytes_freed;

  heap_address = (uint32)ptr;
  heap_base_addr = (MEM_PAGESIZE * 4);
  heap_max_addr = (heap_base_addr + MEM_PAGESIZE - 0x1);
  heap_addr_offset = (heap_address - heap_base_addr);
  physical_heap_address = MemoryTranslateUserToSystem(pcb, heap_address);

  if (ptr == NULL)
  {
    printf ("FATAL: ptr is NULL. Inside mfree\n");
    return -1;
  }

  if ((heap_address < heap_base_addr) || (heap_address > heap_max_addr))
  {
    printf ("FATAL: Inside mfree: ptr does not belong to the heap space.\n");
    //printf ("FATAL: Inside mfree: ptr: %d\n", (int)ptr);
    //printf ("FATAL: Inside mfree: heap_address: %d\n", heap_address);
    //printf ("FATAL: Inside mfree: heap_base_addr: %d\n", heap_base_addr);
    //printf ("FATAL: Inside mfree: heap_max_addr: %d\n", heap_max_addr);
    return -1;
  }

  num_bytes_freed = buddy_mem_free(pcb, heap_addr_offset);
  if (num_bytes_freed == -1)
  {
    printf ("FATAL: Buddy Algorithm failed to find the ptr in Heap Memory. Inside mfree\n");
    return -1;
  }
  else
  {
    //printf ("Freeing heap block of size %d bytes: virtual address 0x%08x, physical address 0x%08x.\n", num_bytes_freed, heap_address, physical_heap_address);
    return num_bytes_freed;
  }
}

