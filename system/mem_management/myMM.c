/**
 * Malloc.c
 * Implicit free list(single linked list, double linked list):
 * 	each block is like double linked list:
 * 	|  size | a|	header
 * 	|          |
 * 	|  payload |	body
 * 	|  padding |
 * 	|  size | a|	footer
 *
 * Explicit free list: (LIFO, insert freed block to the beginning)
 * 	|  size    | a|	header
 * 	|forward link |	forward free block ptr
 * 	|backward link|
 * 	|  payload    |	body
 * 	|  padding    |
 * 	|  size    | a|	footer
 *
 * Segregated free lists:
 *	each size class has its own collection of blocks
 *	4-8 | | | -> | | | -> | | | ->
 *	12 | | | -> | | | ->
 *	16
 *	20-32
 *	Often separate size class for every small size (8, 12, 16, …)
	For larger, typically have size class for each power of 2
	no header, no footer, just payload in that block
	cause massive fragmentation
 * 
 * Segreated fit:
 * 	maintain separate explicit free lists of varying size classes
 * 	dynamically manage blocks in lists
 *
 * 	|  size    | a|	header
 * 	|forward link |
 * 	|backward link|
 *	
 *	malloc: look in the list of size >= k, use first fit, split if possible (threshold), putting leftover on appropriate list
 *	free: free and, if possible coalesce, add block to the appropriate list
 * using first-fit strategy (other strategies: next-fit, best-fit)
 *
 * start at the first block, assuming it exists, and iterate until a block that represents unallocated space of sufficient size is found
 * if no suitable block is found, call sbrk()
 * break free block which is about to allocated into 2 blocks, and add them into implicit list.
 */

/*Macro function: */
