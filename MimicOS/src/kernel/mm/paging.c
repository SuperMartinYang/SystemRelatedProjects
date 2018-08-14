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
}
