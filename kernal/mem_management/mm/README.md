#Memory management

Memory management keeps track of each and every memory location, regardless of either it is allocated to some process or it is free. It checks how much memory is to be allocated to processes. It decides which process will get memory at what time. It tracks whenever some memory gets freed or unallocated and correspondingly it updates the status.

## Process Address Space

OS is responsible for mapping physical addr to logical addr. Three types of addresses used in a program:

| S.N. | Memory Addresses & Description                            |
| ---- | ----------------------------------------------------------|
| 1    | **Symbolic addresses**: The addresses used in a source code. The variable names, constants, and instruction labels are the basic elements of the symbolic address space. |
| 2    | **Relative addresses**: At the time of compilation, a compiler converts symbolic addresses into relative addresses. |
| 3    | **Physical addresses**: The loader generates these addresses at the time when a program is loaded into main memory. |

* MMU mechanism to convert virtual addr to physical addr
  * The value in the base register is added to every address generated by a user process, which is treated as offset at the time it is sent to memory. For example, if the base register value is 10000, then an attempt by the user to use address location 100 will be dynamically reallocated to location 10100.
  * The user program deals with virtual addresses; it never sees the real physical addresses.

## Static vs Dynamic Loading
At the time of loading, with static loading, the absolute program (and data) is loaded into memory in order for execution to start.

If you are using dynamic loading, dynamic routines of the library are stored on a disk in relocatable form and are loaded into memory only when they are needed by the program.

## Static vs Dynamic Linking
As explained above, when static linking is used, the linker combines all other modules needed by a program into a single executable program to avoid any runtime dependency.

When dynamic linking is used, it is not required to link the actual module or library with the program, rather a reference to the dynamic module is provided at the time of compilation and linking. Dynamic Link Libraries (DLL) in Windows and Shared Objects in Unix are good examples of dynamic libraries.

## Swapping
Temporarily swap process from MM to Disk
![Process Swap](https://www.tutorialspoint.com/operating_system/images/process_swapping.jpg)

## Memory Allocation
**2 part**:
	* Low Memory: OS reside
	* High Memory: User process
|S.N.|Memory Allocation & Description|
|------|---------------------------------------------------|
|1            |**Single-partition allocation**: In this type of allocation, relocation-register scheme is used to protect user processes from each other, and from changing operating-system code and data. Relocation register contains value of smallest physical address whereas limit register contains range of logical addresses. Each logical address must be less than the limit register.|
| 2 |	**Multiple-partition allocation**:In this type of allocation, main memory is divided into a number of fixed-sized partitions where each partition should contain only one process. When a partition is free, a process is selected from the input queue and is loaded into the free partition. When the process terminates, the partition becomes available for another process.|

##Fragmentation

| S.N. | Fragmentation & Description                                  |
| ---- | ------------------------------------------------------------ |
| 1    | **External fragmentation**: Total memory space is enough to satisfy a request or to reside a process in it, but it is not contiguous, so it cannot be used. |
| 2    | **Internal fragmentation**: Memory block assigned to process is bigger. Some portion of memory is left unused, as it cannot be used by another process. |

External fragmentation can be reduced by compaction or shuffle memory contents to place all free memory together in one large block. To make compaction feasible, relocation should be dynamic.

The internal fragmentation can be reduced by effectively assigning the smallest partition but large enough for the process.

## Paging
A computer can address more memory than the amount physically installed on the system. This extra memory is actually called virtual memory and it is a section of a hard that's set up to emulate the computer's RAM. Paging technique plays an important role in implementing virtual memory.

![Virtual Memory Organization](https://www.tutorialspoint.com/operating_system/images/paging.jpg)

Pages -> Frames

## Address Translation

```
Logical Address = Page number + page offset
Physical Address = Frame number + page offset
```
![Page Map Table](https://www.tutorialspoint.com/operating_system/images/page_map_table.jpg)

OS will move idle or unwanted pages of memory to secondary memory to free up RAM for other processes and brings them back when needed by the program

## Advantages and Disadvantages
**A list of advantages and disadvantages of paging:**

* Paging reduces external fragmentation, but still suffer from internal fragmentation.
* Paging is simple to implement and assumed as an efficient memory management technique.
* Due to equal size of the pages and frames, swapping becomes very easy.
* Page table requires extra memory space, so may not be good for a system having small RAM.

## Segmentation
Segmentation is a memory management technique in which each job is divided into several segments of different sizes, one for each module that contains pieces that perform related functions. Each segment is actually a different logical address space of the program.
It's similar to paging, but with variable length, while paging pages are of fixed size
![Segmentation Organization](https://www.tutorialspoint.com/operating_system/images/segment_map_table.jpg)

# Virtual Memory
A computer can address more memory than the amount physically installed on the system. This extra memory is actually called virtual memory.
The main visible advantage of this scheme is that programs can be larger than physical memory. First, it allows us to extend the use of physical memory by using disk. Second, it allows us to have memory protection, because each virtual address is translated to a physical address.

## Some situations make use of VM
```
* User written error handling routines are used only when an error occurred in the data or computation.
* Certain options and features of a program may be used rarely.
* Many tables are assigned a fixed amount of address space even though only a small amount of the table is actually used.
* The ability to execute a program that is only partially in memory would counter many benefits.
* Less number of I/O would be needed to load or swap each user program into memory.
* A program would no longer be constrained by the amount of physical memory that is available.
* Each user program could take less physical memory, more programs could be run the same time, with a corresponding increase in CPU utilization and throughput.
```
Actually looks like: (Mapping is the job of MMU)
![VM mapping to MM and Disk](https://www.tutorialspoint.com/operating_system/images/virtual_memory.jpg)

## Demand Paging
note: Demand Segmentation can also be used to implement VM
Swap in while pages on demand, Swap out while pages on idle. OS deal with program context switch is not really put pages back to Secondary Memory, just loading pages required by new program. 
![Swap in and out](https://www.tutorialspoint.com/operating_system/images/demand_paging.jpg)

note: Context Switch
Context Switch Make multiple processes to share a single CPU possible
PCB (Process control block, store the state of running process when blocked)
it includes:
	* Program Counter
	* Scheduling information
	* Base and limit register value
	* Currently used register
	* Changed State
	* I/O State information
	* Accounting information
![context switch graph](https://www.tutorialspoint.com/operating_system/images/context_switch.jpg)

Types of Page fault:
```
	Minor: If there is no mapping yet for the page, but it is a valid address. This could be memory asked for by sbrk(2) but not written to yet meaning that the operating system can wait for the first write before allocating space. 
	Major: If the mapping to the page is not in memory but on disk. do swap.
	Invalid: When you try to write to a non-writable memory address or read to a non-readable memory address.
```
Read-only bit:
	The read-only bit marks the page as read-only
	note: copy-on-write, copy when write is required
Dirty bit:
	page changed
Execution bit (NX): 
	bytes in page executable 
### Advantages and Disadvantages
Ad:
	* Large virtual memory.
	* More efficient use of memory.
	* There is no limit on degree of multiprogramming.
Dis:
	* Number of tables and the amount of processor overhead for handling page interrupts are greater than in the case of the simple paged management techniques.

## Page Replacement Algorithm
Decide which pages to swap out, the algorithms try to minimize the total number of page misses
Reference String is a string of memory references.
Algorithms: 
* FIFO
![FIFO graph](https://www.tutorialspoint.com/operating_system/images/fifo.jpg) 
* Optimal Page Algorithm (OPT / MIN): lowest page fault rate, replace pages which will not be used for the longest period time
![Optimal Page graph](https://www.tutorialspoint.com/operating_system/images/opr.jpg)
* Least Recently Used (LRU) Algorithm: replace pages which has not been used for the longest period time
![LRU graph](https://www.tutorialspoint.com/operating_system/images/lru.jpg)
* Page Buffering Algorithm
	* To get a process start quickly, keep a pool of free frames.
	* On page fault, select a page to be replaced.
	* Write the new page in the frame of free pool, mark the page table and restart the process.
	* Now write the dirty page out of disk and place the frame holding replaced page in free pool.
* Least Frequently Used (LFU) Algorithm: pages with smallest count will be replaced
* Most Frequently Used (MFU) Algorithm: pages with smallest count may just been brought in


### About the mmu emulator

x86-mmu-emu