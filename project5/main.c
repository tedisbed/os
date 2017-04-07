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

int *fault_counters;
int *frame_table;
int nframes, npages;
int frames_in_use = 0, faults = 0, reads = 0, writes = 0;
const char *mode;
char *physmem;
struct disk *disk;

void page_fault_handler(struct page_table *pt, int page)
{
	int frame, bits, tmp_page;
	page_table_get_entry(pt, page, &frame, &bits);
	fault_counters[page]++;

	if (bits == 1) {
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		faults--;
	} else if (frames_in_use < nframes) {
		page_table_set_entry(pt, page, frames_in_use, PROT_READ);
		disk_read(disk, page, &physmem[frames_in_use * PAGE_SIZE]);
		frame_table[frames_in_use] = page;
		reads++;
		frames_in_use++;
	} else if (!strcmp(mode, "rand")) {
		int rand_frame = rand() % nframes;
		tmp_page = frame_table[rand_frame];
		page_table_get_entry(pt, tmp_page, &frame, &bits);
		if (bits == 3) {
			disk_write(disk, tmp_page, &physmem[frame * PAGE_SIZE]);
			writes++;
		}
		disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
		frame_table[frame] = page;
		reads++;
		page_table_set_entry(pt, page, frame, PROT_READ);
		page_table_set_entry(pt, tmp_page, 0, 0);
	} else if (!strcmp(mode, "fifo")) {
		int first_frame = frames_in_use % nframes;
		frames_in_use++;
		tmp_page = frame_table[first_frame];
		page_table_get_entry(pt, tmp_page, &frame, &bits);
		if (bits == 3) {
			disk_write(disk, tmp_page, &physmem[frame * PAGE_SIZE]);
			writes++;
		}
		disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
		frame_table[frame] = page;
		reads++;
		page_table_set_entry(pt, page, frame, PROT_READ);
		page_table_set_entry(pt, tmp_page, 0, 0);
	} else if (!strcmp(mode, "custom")) {
		int tmp_page_2, x;
		int min = 100000;
		for (x = 0; x < nframes; x++) {
			tmp_page_2 = frame_table[x];
			if (fault_counters[tmp_page_2] <= min) {
				tmp_page = tmp_page_2;
				min = fault_counters[tmp_page_2];
			}
		}
		page_table_get_entry(pt, tmp_page, &frame, &bits);
		if (bits == 3) {
			disk_write(disk, tmp_page, &physmem[frame * PAGE_SIZE]);
			writes++;
		}
		disk_read(disk, page, &physmem[frame * PAGE_SIZE]);
		frame_table[frame] = page;
		reads++;
		page_table_set_entry(pt, page, frame, PROT_READ);
		page_table_set_entry(pt, tmp_page, 0, 0);
	}

	faults++;
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

	int x;
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
	free(fault_counters);
	free(frame_table);

	printf("Page faults: %d\nDisk reads: %d\nDisk writes: %d\n", faults, reads, writes);
	return 0;
}
