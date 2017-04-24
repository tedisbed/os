/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/


// Import Modules

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


// Global Variables

int *fault_counters;
int *frame_table;
int nframes, npages;
int frames_in_use = 0, faults = 0, reads = 0, writes = 0;
const char *mode;
char *physmem;
struct disk *disk;


// Page Fault Handler

void page_fault_handler(struct page_table *pt, int page)
{
	int frame, bits, page_to_remove;
	page_table_get_entry(pt, page, &frame, &bits); // get current status of page
	fault_counters[page]++; // increment fault counter for custom algorithm
	faults++;

	if (bits == 1) { // if the read bit is already set, it's a permissions fault
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE); // so add write permission
		faults--; // and don't count it as a real fault
		return;
	} else if (frames_in_use < nframes) { // if the frames aren't full
		page_table_set_entry(pt, page, frames_in_use, PROT_READ); // set the entry
		disk_read(disk, page, &physmem[frames_in_use * PAGE_SIZE]); // read in the data
		frame_table[frames_in_use] = page; // adjust frame table
		reads++; // increment read counter and frames in use counter
		frames_in_use++;
		return;
	} else if (!strcmp(mode, "rand")) {
		int rand_frame = rand() % nframes; // pick a random frame in the range
		page_to_remove = frame_table[rand_frame];
	} else if (!strcmp(mode, "fifo")) {
		int first_frame = frames_in_use % nframes; // pick the next frame based on FIFO order
		frames_in_use++;
		page_to_remove = frame_table[first_frame];
	} else if (!strcmp(mode, "custom")) {
		int tmp_page_2, x; // pick the frame with the page that has faulted the least so far
		int min = 100000000;
		for (x = 0; x < nframes; x++) {
			tmp_page_2 = frame_table[x];
			if (fault_counters[tmp_page_2] <= min) {
				page_to_remove = tmp_page_2;
				min = fault_counters[tmp_page_2];
			}
		}
	}

	page_table_get_entry(pt, page_to_remove, &frame, &bits); // get the page's info
	if (bits == 3) { // if it was written to, write back to disk
		disk_write(disk, page_to_remove, &physmem[frame * PAGE_SIZE]);
		writes++; // and increment the write counter
	}
	disk_read(disk, page, &physmem[frame * PAGE_SIZE]); // read in the new data
	frame_table[frame] = page; // update frame table
	reads++; // increment read counter
	page_table_set_entry(pt, page, frame, PROT_READ); // update the page table
	page_table_set_entry(pt, page_to_remove, 0, 0);
}


// Main Execution

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	npages = atoi(argv[1]); // assign variables from command line
	nframes = atoi(argv[2]);
	if (npages == 0 || nframes == 0) { // catch invalid arguments
		fprintf(stderr, "error: invalid npages or nframes argument (must be number greater than 0)\n");
		return 1;
	}
	mode = argv[3];
	if(strcmp(mode, "rand") && strcmp(mode, "fifo") && strcmp(mode, "custom")) {
		fprintf(stderr, "unknown algorithm mode: %s\n", argv[3]);
		return 1;
	}
	const char *program = argv[4];

	int x; // initiate dynamically allocated data
	frame_table = malloc(nframes * sizeof(int));
	for (x = 0; x < nframes; x++) {
		frame_table[x] = -1;
	}
	fault_counters = malloc(npages * sizeof(int));
	for (x = 0; x < npages; x++) {
		fault_counters[x] = 0;
	}

	disk = disk_open("myvirtualdisk",npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}

	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n", argv[4]);
		return 1;
	}

	page_table_delete(pt);
	disk_close(disk);
	free(fault_counters); // free all allocated memory
	free(frame_table);

	// print results
	printf("Page faults: %d\nDisk reads: %d\nDisk writes: %d\n", faults, reads, writes);
	return 0;
}
