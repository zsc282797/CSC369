#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int clock_hand;
int* clock_ref_int;
int clock_evict() {
	int i=0;
	for (i = clock_hand; i < memsize; i = (i+1) % memsize){
		
		if(clock_ref_int[i]==1) { //referenced 
			clock_ref_int[i]=0; //second chance 
		}
		else { //clock_ref[i] == 0
			clock_ref_int[i]=1; 
			clock_hand = i;
			return i;
		}
	}
	return i;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	int frame_num = p->frame >> PAGE_SHIFT;
	clock_ref_int[frame_num] = 1; //set as referenced 
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock_hand = 0; 
	clock_ref_int = malloc(sizeof(int) * memsize);
	int i;
	for (i=0;i<memsize;i++){
		clock_ref_int[i] = 0; //initialize to not referenced 
	}
}
