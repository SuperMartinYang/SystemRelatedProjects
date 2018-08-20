#include <kernel/mm/paging.h>
#include <kernel/mm/physical.h>
#include <kernel/mm/dma.h>
#include <kernel/mm/mm.h>
#include <kernel/kernel.h>
#include <kernel/interrupt.h>
#include <kernel/pm/scheduler.h>
#include <kernel/pm/process.h>
#include <kernel/pm/sync/mutex.h>
#include <lib/libc/string.h>

#include <kernel/debug.h>

extern void start;
extern void end;

extern struct PROCESS_INFO kernel_process;

struct MUTEX paging_lock;

void paging_setCurrentPageDir(struct PAGE_DIRECTORY * pd){
    // set cr3 to the physical address of the page directory
    ASM("movl %0, %%cr3" :: "r" (pd));
}

struct PAGE_DIRECTORY_ENTRY * paging_getPageDirectoryEntry(struct PAGE_DIRECTORY * pd, void * linearAddress){
    // convert the physical address of pd into a virtual address before reading/writing
    pd = (struct PAGE_DIRECTORY *)paging_mapQuick(pd);
    return &pd->entry[GET_DIRECTORY_INDEX(linearAddress)];
}

void paging_setPageDirectoryEntry(struct PROCESS_INFO * p, void * linearAddress, struct PAGE_TABLE * pt){
    struct PAGE_DIRECTORY_ENTRY * pde = paging_getPageDirectoryEntry(p->page_dir, linearAddress);
    pde->present        = TRUE;
    pde->readwrite      = READWRITE;
    pde->user           = p->privilege;
    pde->writethrough   = 0;
    pde->cachedisabled  = 0;
    pde->accessed       = 0;
    pde->reserved       = 0;
    pde->pagesize       = 0;
    pde->globalpage     = 0;
    pde->available      = 0;
    pde->address        = TABLE_SHIFT_R(pt);
}

void page_setPageTableEntry(struct PROCESS_INFO *, void *, void *, BOOL);

struct PAGE_TABLE_ENTRY * paging_getPageTableEntry(struct PROCESS_INFO * p, void * linearAddress){
    struct PAGE_DIRECTORY_ENTRY * pde = paging_getPageDirectoryEntry(p->page_dir, linearAddress);
    struct PAGE_TABLE * pt = (struct PAGE_TABLE *)(TABLE_SHIFT_L(pde->address));
    if (pt == NULL){
        int index;
        pt = (struct PAGE_TABLE *)physical_pageAlloc();
        paging_setPageDirectoryEntry(p, linearAddress, pt);
        for (index = 0; index < PAGE_ENTRYS; index ++){
            // although it will use "get", but it won't arrive here again.
            paging_setPageTableEntry(p, linearAddress + (index * SIZE_4KB), NULL, FALSE);
        }
    }
    // convert the physical address of pt into a virtual address before reading/writing
    pt = (struct PAGE_TABLE *)paging_mapQuick(pt);
    // return the entry which is now a virtual address in the current address space
    return (struct PAGE_TABLE_ENTRY *)&pt->entry[GET_TABLE_INDEX(linearAddress)];
}

void paging_setPageTableEntry(struct PROCESS_INFO * p, void * linearAddress, void * physicalAddress, BOOL present){
    struct PAGE_TABLE_ENTRY * pte = paging_getPageTableEntry(p, PAGE_ALIGN(linearAddress));
    pte->present        = present;
    pte->readwrite      = READWRITE;
    pte->user           = p->privilege;
    pte->writethrough   = 0;
    pte->cachedisabled  = 0;
    pte->accessed       = 0;
    pte->dirty          = 0;
    pte->attributeindex = 0;
    pte->globalpage     = 0;
    pte->available      = 0;
    pte->address        = TABLE_SHIFT_R(PAGE_ALIGN(physicalAddress));
}

void PROCESS_INFO * paging_pageFaultHandler(struct PROCESS_INFO * process){
    void * linearAddress;
    // retrieve the linear address of the page fault stored in CR2
    ASM("movl %%cr2, %0" : "=r" (linearAddress));
    kernel_printf("Page Fault at CS:EIP %x:%x Address %x\n", process->kstack->cs, process->kstack->eip, linearAddress);
    // if the kernel caused the page fault we must kernel panic
    if (process->id == KERNEL_PID)
        kernel_panic(process->kstack, "Kernel Page Fault.");
    // print out the stack
    process_printStack(process->kstack);
    // try to kill the offending process
    if (process_kill(process_id) == SUCCESS)
        return schedular_select(NULL);
    // if we filled to kill the process, we don't need to perform a context switch
    return process;
}

int paging_createDirectory(struct PROCESS_INFO * p){
    int index;
    struct PAGE_DIRECTORY * pd;
    // lock
    mutex_lock(&paging_lock);
    // allocate some physical memory for the page directory
    p->page_dir = (struct PAGE_DIRECTORY *)physical_pageAlloc();
    if (p->page_dir == NULL){
        mutex_unlock(&paging_lock);
        return FAIL;
    }
    // map the p->page_dir physical address into the current address space so we can read/write to it
    pd = (struct PAGE_DIRECTORY *)paging_mapQuick(p->page_dir);
    memset(pd, 0x00, sizeof(struct PAGE_DIRECTORY));
    // set some default entrys in the page directory
    for (index = 0; index < PAGE_ENTRYS; index ++){
        // get the next entry
        struct PAGE_DIRECTORY_ENTRY * pde = &pd->entry[index];
        // set the privilege to the process
        pde->user = p->privilege;
        // if it's the last entry
        if (index == PAGE_ENTRYS - 1){
            // store the page dir as the last entry in itself( fractal mapping )
            pde->present = TRUE;
            pde->readwrite = READWRITE;
            pde->address = TABLE_SHIFT_R(p->page_dir);
        }else {
            pde->present = FALSE;
            pde->readwrite = READONLY;
        }
    }
    // unlock
    mutex_unlock(&paging_lock);
    return SUCCESS;
}

//TO-DO
void paging_destroyDirectory(struct PROCESS_INFO * p){

}