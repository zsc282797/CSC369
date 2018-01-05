#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int * refer;  //the age of the pages 
int lru_evict() {
	int oldest = 0;
	int old_frame = 0;
	int i =0; 
	for (i=0;i<memsize;i++){
		if (refer[i] > oldest){
			oldest = refer[i];
			old_frame = i;
		}
	}
	return old_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
	int frame_num = p->frame >> PAGE_SHIFT; 
	int i = 0;
	for (i =0; i<memsize;i++){
		if (refer[i] > 0){
			refer[i] ++; //
		}
	}
	refer[frame_num] = 1; 
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	refer = malloc(sizeof(int) * memsize);
	int i = 0;
	for (i =0 ;i<memsize;i++){
		refer[i] = 0;
	}
}
