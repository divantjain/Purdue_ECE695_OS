#ifndef	_memory_h_
#define	_memory_h_

// Put all your #define's in memory_constants.h
#include "memory_constants.h"

extern int lastosaddress; // Defined in an assembly file

//--------------------------------------------------------
// Existing function prototypes:
//--------------------------------------------------------

int MemoryGetSize();
void MemoryModuleInit();
uint32 MemoryTranslateUserToSystem (PCB *pcb, uint32 addr);
int MemoryMoveBetweenSpaces (PCB *pcb, unsigned char *system, unsigned char *user, int n, int dir);
int MemoryCopySystemToUser (PCB *pcb, unsigned char *from, unsigned char *to, int n);
int MemoryCopyUserToSystem (PCB *pcb, unsigned char *from, unsigned char *to, int n);
int MemoryPageFaultHandler(PCB *pcb);

//---------------------------------------------------------
// Put your function prototypes here
//---------------------------------------------------------
// All function prototypes including the malloc and mfree functions go here

uint32 MemoryAllocPage (void);
uint32 MemorySetupPte (uint32 page);
void MemoryFreePage (uint32 page);
void MemoryFreePte (uint32 pte);

// Heap Management Functions
int CalculatePower(int base, int power);
void PrintHeapNode(heap_node *node);
void AssignHeapNode (heap_node *node, int order, uint32 addr, int size, int created, int occupied, int splitted, heap_node *parent, heap_node *left, heap_node *right, heap_node *buddy);
void InitializeHeapTree (PCB *pcb);
void FreeHeapTree (PCB *pcb);
int GetRequiredOrder (int memsize);
void CreteTwoBuddyNodes (heap_node *node);
void CreateNode(heap_node *node);
uint32 SearchAndAllocateNodeForOrder (PCB *pcb, int order_required, int memsize);
uint32 buddy_mem_alloc(PCB *pcb, int memsize);

void CombineBuddies(heap_node *node);
void CheckAndCombineFreeBuddies(heap_node *node);
int SearchForNodeToBeFreed (PCB *pcb, int heap_addr_offset);
int buddy_mem_free(PCB *pcb, int heap_addr_offset);

void *malloc (PCB *pcb, int memsize);
int mfree (PCB *pcb, void *ptr);

#endif	// _memory_h_
