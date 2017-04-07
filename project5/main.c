/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int *custom_queue;
int nframes, npages;
int frames_in_use = 0;
const char *mode;
char *physmem;
struct disk *disk;

void page_fault_handler(struct page_table *pt, int page)
{
	int frame, bits, x;
	page_table_get_entry(pt, page, &frame, &bits);

	if (bits == 1) {
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		int limit;
		if (frames_in_use <= nframes) {
			limit = frames_in_use;
		} else {
			limit = nframes;
		}
		int start_swapping = 0;
		for (x = 0; x < limit - 1; x++) {
			if (custom_queue[x] == frame) {
				start_swapping = 1;
			}
			if (start_swapping == 1) {
				custom_queue[x] = custom_queue[x+1];
			}
		}
		custom_queue[limit-1] = frame;
	} else if (frames_in_use < nframes) {
		page_table_set_entry(pt, page, frames_in_use, PROT_READ);
		disk_read(disk, page, &physmem[frames_in_use * PAGE_SIZE]);
		custom_queue[frames_in_use] = frame;
		frames_in_use++;
	} else if (!strcmp(mode, "rand")) {
		int rand_frame = rand() % nframes;
		for (x = 0; x < npages; x++) {
			page_table_get_entry(pt, x, &frame, &bits);
			if (frame == rand_frame && bits != 0) {
				if (bits == 3) {
					disk_write(disk, x, &physmem[frame * PAGE_SIZE]);
				}
				disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
				page_table_set_entry(pt, page, frame, PROT_READ);
				page_table_set_entry(pt, x, 0, 0);
				break;
			}
		}
	} else if (!strcmp(mode, "fifo")) {
		int first_frame = frames_in_use % nframes;
		frames_in_use++;
		for (x = 0; x < npages; x++) {
			page_table_get_entry(pt, x, &frame, &bits);
			if (frame == first_frame && bits != 0) {
				if (bits == 3) {
					disk_write(disk, x, &physmem[frame * PAGE_SIZE]);
				}
				disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
				page_table_set_entry(pt, page, frame, PROT_READ);
				page_table_set_entry(pt, x, 0, 0);
				break;
			}
		}
	} else if (!strcmp(mode, "custom")) {
		int top_frame = custom_queue[0];
		for (x = 0; x < npages; x++) {
			page_table_get_entry(pt, x, &frame, &bits);
			if (frame == top_frame && bits != 0) {
				if (bits == 3) {
					disk_write(disk, x, &physmem[frame * PAGE_SIZE]);
				}
				disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
				page_table_set_entry(pt, page, frame, PROT_READ);
				page_table_set_entry(pt, x, 0, 0);
			}
		}

		for (x = 0; x < nframes - 1; x++) {
			custom_queue[x] = custom_queue[x+1];
		}
		custom_queue[nframes - 1] = top_frame;
	}

	printf("page fault on page #%d\n", page);
	//exit(1);
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>\n");
		return 1;
	}

	npages = atoi(argv[1]);
	nframes = atoi(argv[2]);
	mode = argv[3];
	if(strcmp(mode, "rand") && strcmp(mode, "fifo") && strcmp(mode, "custom")) {
		fprintf(stderr, "unknown algorithm mode: %s\n", argv[3]);
		return 1;
	}
	const char *program = argv[4];

	custom_queue = malloc(nframes * sizeof(int));

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
	free(custom_queue);

	return 0;
}
