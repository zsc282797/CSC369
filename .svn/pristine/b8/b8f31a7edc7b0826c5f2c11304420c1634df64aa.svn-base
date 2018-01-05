#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h"



extern int debug;

extern struct frame *coremap;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
addrNode_t * head;
addrNode_t * curr;
addrNode_t * tail;
//pointers to travers the linked list 
int opt_evict() {
	addr_t far_vaddr; //for looping through addresses 
	int i = 0; int farthest_dis = -1000; int farthest_frame = -1; 
	for (i = 0; i< memsize; i++){ //loop through the core map 
		far_vaddr = coremap[i].vaddr;
		
		addrNode_t * temp = curr->next; //point to start looking 
		int dis = 0; 
		while (temp!=NULL && temp->vaddr!=far_vaddr){
			
			dis++;
			temp = temp->next; //find the distance of this address 
		}

		if (temp!=NULL) { //the pointer did not go over the list 
			if (dis>farthest_dis){
				farthest_dis = dis;
				farthest_frame = i;
			}

		}
		else {
			
			return i; 
		} // exhasted the list but no repeat 
	}
	

	
	return farthest_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	//Since order is garuteed to be the same as in the trace 
	if (curr->next){
		curr = curr->next;
	}
	else {curr = head; } //move to the next if not NULL otherwise back to head 
	return; 
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	FILE * trace_file = fopen(tracefile,"r"); //tracefile is extern 
	
	//Some of the following reading code is copied from sim.c 
	char buf[MAXLINE];
	addr_t vaddr = 0;
	char type;
	head = NULL; curr = NULL; tail = NULL;

	while(fgets(buf, MAXLINE, trace_file) != NULL) {
		if(buf[0] != '=') {
			sscanf(buf, "%c %lx", &type, &vaddr);
			if(debug)  {
				printf("%c %lx\n", type, vaddr);
			}
		addrNode_t* new_addr = malloc(sizeof(addrNode_t));
		vaddr &= ~0xF; //clear the status bits 
		new_addr -> vaddr = vaddr;
		new_addr -> next = NULL;

		if (!head){
			head = new_addr;
			tail = head; //create the list 
		}
		else {
			tail->next = new_addr;
			tail = tail->next; //add to the tail 
		}
		} else {
			continue;
		}

	}
	curr = head;

}

