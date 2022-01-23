#ifndef	_memory_constants_h_
#define	_memory_constants_h_

//------------------------------------------------
// #define's that you are given:
//------------------------------------------------

// We can read this address in I/O space to figure out how much memory
// is available on the system.
#define	DLX_MEMSIZE_ADDRESS	0xffff0000

// Return values for success and failure of functions
#define MEM_SUCCESS 1
#define MEM_FAIL -1

//--------------------------------------------------------
// Put your constant definitions related to memory here.
// Be sure to prepend any constant names with "MEM_" so 
// that the grader knows they are defined in this file.

//--------------------------------------------------------
// bit position of the least significant bit of the level 1 page number field in a virtual address.
#define MEM_L1FIELD_FIRST_BITNUM 14
// bit position of the least significant bit of the level 2 page number field in a virtual address. 
#define MEM_L2FIELD_FIRST_BITNUM 12

// the maximum allowable address in the virtual address space. Note that this is not the 4-byte-aligned address, but rather the actual maximum address (it should end with 0xF).
#define MEM_MAX_VIRTUAL_ADDRESS 0xfffff
#define MEM_L1_PTSIZE ((MEM_MAX_VIRTUAL_ADDRESS + 0x1) >> MEM_L1FIELD_FIRST_BITNUM)

#define MEM_L2_PTSIZE (0x1 << (MEM_L1FIELD_FIRST_BITNUM - MEM_L2FIELD_FIRST_BITNUM))
#define MEM_NUM_L2_PT_FREEMAP (MEM_L2_PTSIZE / 32)

#define MEM_PAGESIZE (0x1 << MEM_L2FIELD_FIRST_BITNUM)
#define MEM_ADDRESS_OFFSET_MASK (MEM_PAGESIZE - 1)
#define MEM_NUM_VIR_PAGES ((MEM_MAX_VIRTUAL_ADDRESS + 0X1) >> MEM_L2FIELD_FIRST_BITNUM)

// the maximum physical memory size (2MB)
#define MEM_MAX_SIZE 0x1fffff
// Num physical pages 
#define MEM_NUM_PHY_PAGES ((MEM_MAX_SIZE + 0x1) >> MEM_L2FIELD_FIRST_BITNUM)
#define MEM_NUM_PHY_PAGES_FREEMAP (MEM_NUM_PHY_PAGES / 32)

#define MEM_PTE_READONLY 0x00000004
#define MEM_PTE_DIRTY 0x00000002
#define MEM_PTE_VALID 0x00000001
#define MEM_PTE2PADDR_MASK (~(MEM_PTE_READONLY | MEM_PTE_DIRTY | MEM_PTE_VALID))

#endif	// _memory_constants_h_
