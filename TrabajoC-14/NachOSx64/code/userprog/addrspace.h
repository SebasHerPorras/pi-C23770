// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"

#define UserStackSize		1024 	// increase this as necessary!

int VirtualToPhysical(int virtualAddr, TranslationEntry* pageTable); // this function converts virtual addresses to physical addresses Sebas implementation
void LoadSegment(OpenFile* executable, int virtualAddr, int fileOffset, int size, TranslationEntry* pageTable, int numPages); // this function loads a segment from the executable into memory
class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
    AddrSpace();
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch 
    unsigned int getNumPages() { return numPages; } // this function returns the number of pages in the address space
    TranslationEntry* getPageTable() { return pageTable; } // this function returns the page table of the address space
    AddrSpace* Clone();		// this function creates a clone of the address space
    int AllocateMemory(int size); // this function allocates memory in the address space
    void Close(int fd); // this function closes the file descriptor
    private:
    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    unsigned int numPages;		// Number of pages in the virtual 
    int nextFreeVirtualAddr;  // Dirección virtual desde donde empieza el heap libre

					// address space
};

#endif // ADDRSPACE_H
