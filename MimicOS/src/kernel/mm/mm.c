#include <kernel/mm/mm.h>
#include <kernel/mm/physical.h>
#include <kernel/mm/segmentation.h>
#include <kernel/mm/paging.h>
#include <kernel/pm/process.h>
#include <kernel/pm/sync/mutex.h>
#include <kernel/interrupt.h>
#include <kernel/kernel.h>
#include <lib/libc/string.h>

extern struct PROCESS_INFO kernel_process;

struct MUTEX mm_kmallocLock;

int mm_init(struct MULTIBOOT_INFO * m){
    // setup the physical memory manager, after this we can use physical_pageAlloc()
    physical_init(m);

    // setup and initialize segmentation
    segmentation_init();

    // setup and initialize paging
    paging_init();

    // setup the kernel heap structure
    kernel_process.heap.heap_base = KERNEL_HEAP_VADDRESS;
    kernel_process.heap.heap_top = NULL;

    mutex_init(&mm_kmallocLock);

    return SUCCESS;

}

// precess memory copy from dst to src
void mm_pmemcpyto(void * dst_paddress, void * src_vaddress, int size){
    // disable interrupts for atomicity
    interrupt_disableAll();
    // use quickMap() to map in the physical page into our current address space so we may write to it
    void * tmpAddress = paging_mapQuick(dst_paddress);
    memcpy(tmpAddress, src_vaddress, size);
    // enable interrupt
    interrupt_enableAll();
}

void mm_pmemcpyfrom(void * dst_vaddress, void src_paddress, int size){
    interrupt_disableAll();
    void * tmpAddress = paging_mapQuick(src_paddress);
    memcpy(dst_vaddress, tmpAddress, size);
    interrupt_enableAll();
}

// increase the processes 
void * mm_morecore(struct PROCESS_INFO * process, DWORD size){

}