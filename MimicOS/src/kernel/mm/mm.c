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

// increase the processes heap by some amount, this will be rounded up to the page size
void * mm_morecore(struct PROCESS_INFO * process, DWORD size){
    // calculate how many page we will need
    int pages = (size / PAGE_SIZE) + 1;
    // when process->heap.heap_top == NULL, we must create the initial heap
    if (process->heap.heap_top == NULL)
        process->heap.heap_top = process->heap.heap_base;
    void * prevTop = process->heap.heap_top;
    // create the page
    for (; pages-- > 0; process->heap.heap_top += PAGE_SIZE){
        // alloc a physical page in memory
        void * physicalAddress = physical_pageAlloc();
        // map it onto the end of the process heap
        paging_map(process, process->heap.heap_top, physicalAddress, TRUE);
        memset(process->heap.heap_top, 0x00, PAGE_SIZE);
    }
    // return the start address of the memory we allocated to the heap
    return prevTop;
} 

// free a previously allocated item from the kernel heap
void mm_kfree(void * address){
    struct MM_HEAPITEM * tmp_item, * item;
    // sanity check
    if (addresss == NULL)
        return;
    // lock this critical section
    mutex_lock(&mm_kmallocLock);
    // set the item to remove
    item = (struct MM_HEAPITEM *)(address - sizeof(struct MM_HEAPITEM));
    for (tmp_item = kernel_process.heap.heap_base; tmp_item != NULL; tmp_item = tmp_item->next){
        if (tmp_item == item){
            // free it
            tmp_item->used = FALSE;
            // coalesce any adjacent free items
            for (tmp_item = kernel_process.heap.heap_base; tmp_item != NULL; tmp_item = tmp_item->next){
                while (!tmp_item->used && tmp_item->next != NULL && !tmp_item->next->used){
                    tmp_item->size += sizeof(struct MM_HEAPITEM) + tmp_item->next->size;
                    tmp_item->next = tmp_item->next->next;
                }
            }
            // break and return as we are finished
            break;
        }
    }
    // unlock this critical section
    mutex_unlock(&mm_kmallocLock);	
}

// allocates an arbiturary size of memory (via first fit) from the kernel heap
// TO-DO: optimize structure
void * mm_kmalloc(DWORD size){
    struct MM_HEAPITEM * new_item = NULL, * tmp_item;
    int total_size;
    // sanity check
    if (size == 0)
        return NULL;
    // lock the critical section
    mutex_lock(&&mm_kmallocLock);
    // round up by 8 bytes and add header size
    total_size = ((size + 7) & ~7) + sizeof(struct MM_HEAPITEM);
    if (kernel_process.heap.heap_top != NULL){
        // search for first fit
        for (new_item=kernel_process.heap.heap_base; new_item != NULL; new_item = new_item->next){
            if (!new_item->used && (total_size <= new_item->size))
                break;
        }
    }
    // if we found one
    if (new_item != NULL){
        tmp_item = (struct MM_HEAPITEM *)((int)new_item + tot_size);
        tmp_item->size = new_item->size - total_size;
        tmp_item->used = FALSE;
        tmp_item->next = new_item->next;
    }else {
        // didn't find a fit so we must increase the heap to fit
        new_item = mm_morecore(&kernel_process, total_size);
        if (new_item == NULL){
            // failed
            mutex_unlock(&mm_kmallocLock);
            return NULL;
        }
        // create an empty item for the extra space 
        // we can calculate the size because morecore() allocates space that is page aligned
        tmp_item = (struct MM_HEAPITEM *)((int)new_item + total_size);
        tmp_item->size = PAGE_SIZE - (total_size % PAGE_SIZE ? total_size % PAGE_SIZE : total_size) - sizeof(struct MM_HEAPITEM);
        tmp_item->used = FALSE;
        tmp_item->next = NULL;
    }
    // create the new item
    new_item->size = size;
    new_item->used = TURE;
    new_item->next = tmp_item;
    // unlock
    mutex_unlock(&mm_kmallocLock);
    // return newly allocated memory location, after heap header, data area
    return (void *)((int)new_item + sizeof(struct MM_HEAPITEM));
}









