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

uint32 GetL2PtIndexBits (uint32 vaddr);
uint32 GetL2PtIndexFromL2PtBaseAddr (uint32 l2_pt_base_addr);
uint32 GetFreeL2Pt ();
uint32 MemoryAllocL2PtForPage (PCB *pcb, uint32 l1_pt_index, uint32 l2_pt_index);
void MemoryAllocForPageAndSetL1PtAndL2Pt (PCB *pcb, uint32 vaddr);
void MemoryFreeL2Pt (uint32 l2_pt_base_addr);

int malloc (PCB *pcb, int memsize);
int mfree (PCB *pcb, void *ptr);

#endif	// _memory_h_
