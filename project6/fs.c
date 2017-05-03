// Teddy Brombach and Tristan Mitchell
// OS Project 6


// Import Modules

#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>


// Define Constants and Structs

#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

struct fs_bitmap {
	int *bits;
	int in_use; // set to 1 when the FS has been mounted
};


// Set Global Bitmap Variables

struct fs_bitmap inode_bitmap = {.in_use = 0};
struct fs_bitmap block_bitmap = {.in_use = 0};


// Function Definitions

int fs_format()
{
	if (block_bitmap.in_use) { // if already mounted, return
		return 0;
	}

	union fs_block block;
	disk_read(0, block.data); // set superblock
	block.super.magic = FS_MAGIC;
	block.super.nblocks = disk_size();
	block.super.ninodeblocks = block.super.nblocks / 10;
	if (block.super.nblocks % 10 != 0) {
		block.super.ninodeblocks++;
	}
	block.super.ninodes = block.super.ninodeblocks * INODES_PER_BLOCK;
	disk_write(0, block.data);

	int i;
	char buffer[DISK_BLOCK_SIZE] = {"\0"}; // empty out all other blocks
	for (i = 1; i < block.super.nblocks; i++) {
		disk_write(i, buffer);
	}

	return 1;
}

void fs_debug()
{
	union fs_block block, block2, block3;

	disk_read(0,block.data);

	printf("superblock:\n"); // show superblock stats
	if (block.super.magic == FS_MAGIC) {
		printf("    magic number is valid\n");
	} else {
		printf("    magic number is invalid\n");
	}
	printf("    %d blocks on disk\n",block.super.nblocks);
	printf("    %d blocks for inodes\n",block.super.ninodeblocks);
	printf("    %d inodes total\n",block.super.ninodes);

	int i, j; // go through inode block data
	for (i = 1; i < block.super.ninodeblocks + 1; i++) {
		disk_read(i, block2.data);
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			if (block2.inode[j].isvalid) {
				printf("inode %d:\n", (i - 1) * INODES_PER_BLOCK + j);
				printf("    size: %d bytes\n", block2.inode[j].size);
				printf("    direct blocks:");

				int k, limit;
				int indirect_needed = 0;
				if (block2.inode[j].size > POINTERS_PER_INODE * DISK_BLOCK_SIZE) {
					limit = POINTERS_PER_INODE;
					indirect_needed = 1;
				} else {
					limit = (block2.inode[j].size / DISK_BLOCK_SIZE) + 1;
					if (block2.inode[j].size % DISK_BLOCK_SIZE == 0) {
						limit--;
					}
				}
				for (k = 0; k < limit; k++) {
					printf(" %d", block2.inode[j].direct[k]);
				}
				printf("\n");

				if (indirect_needed) {
					printf("    indirect block: %d\n", block2.inode[j].indirect);
					printf("    indirect data blocks:");
					disk_read(block2.inode[j].indirect, block3.data);
					int m;
					for (m = 0; m < POINTERS_PER_BLOCK; m++) {
						if (block3.pointers[m] == 0) {
							break;
						} else {
							printf(" %d", block3.pointers[m]);
						}
					}
					printf("\n");
				}
			}
		}
	}
}

int fs_mount()
{
	union fs_block block, block2, block3;
	disk_read(0, block.data);
	if (block.super.magic != FS_MAGIC || block_bitmap.in_use) { // catch invalid FSs and mounted ones
		return 0;
	}

	inode_bitmap.bits = calloc(block.super.ninodeblocks * INODES_PER_BLOCK, sizeof(int));
	block_bitmap.bits = calloc(disk_size(), sizeof(int)); // allocate bitmap space
	block_bitmap.in_use = 1; // set mounted variable
	int i, j, k;
	for (i = 0; i < 1 + block.super.ninodeblocks; i++) {
		block_bitmap.bits[i] = 1; // set bitmap as full for superblock and inode data
		if (i != 0) {
			disk_read(i, block2.data);
			for (j = 0; j < INODES_PER_BLOCK; j++) {
				if (block2.inode[j].isvalid) {
					inode_bitmap.bits[(i - 1) * INODES_PER_BLOCK + j] = 1;
				}
			}
		}
	}

	for (i = 1; i < 1 + block.super.ninodeblocks; i++) {
		disk_read(i, block2.data);
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			if (block2.inode[j].isvalid) {
				int indirect_needed = 0;
				int limit;
				if (block2.inode[j].size > POINTERS_PER_INODE * DISK_BLOCK_SIZE) {
					limit = POINTERS_PER_INODE;
					indirect_needed = 1;
				} else {
					limit = (block2.inode[j].size / DISK_BLOCK_SIZE) + 1;
				}
				for (k = 0; k < limit; k++) {
					block_bitmap.bits[block2.inode[j].direct[k]] = 1;
				}

				if (indirect_needed) {
					block_bitmap.bits[block2.inode[j].indirect] = 1;
					disk_read(block2.inode[j].indirect, block3.data);
					int m;
					for (m = 0; m < POINTERS_PER_BLOCK; m++) {
						if (block3.pointers[m] == 0) {
							break;
						} else {
							block_bitmap.bits[block3.pointers[m]] = 1;
						}
					}
				}
			}
		}
	}

	return 1;
}

int fs_create()
{
	if (!block_bitmap.in_use) { // catch unmounted FSs
		return 0;
	}

	union fs_block block, block2;
	disk_read(0, block.data);

	int j;
	int inumber = 0;
	for (j = 1; j < block.super.ninodeblocks * INODES_PER_BLOCK; j++) { // find first free inode
		if (inode_bitmap.bits[j] == 0) {
			inode_bitmap.bits[j] = 1; // mark it as used
			inumber = j;
			break;
		}
	}

	if (inumber != 0) { // initialize the inode if an empty one was found
		int inode_index = inumber / INODES_PER_BLOCK + 1;
		disk_read(inode_index, block2.data);
		block2.inode[inumber % INODES_PER_BLOCK].isvalid = 1;
		block2.inode[inumber % INODES_PER_BLOCK].size = 0;
		int x;
		for (x = 0; x < POINTERS_PER_INODE; x++) {
			block2.inode[inumber % INODES_PER_BLOCK].direct[x] = 0;
		}
		block2.inode[inumber % INODES_PER_BLOCK].indirect = 0;
		disk_write(inode_index, block2.data);
	}

	return inumber;
}

int fs_delete( int inumber )
{
	if (!block_bitmap.in_use) { // catch mounted FSs
		return 0;
	}

	// clear out all used space, set bitmap data back to empty
	union fs_block block, block2;
	disk_read(0, block.data);
	if (inumber <= 0 || inumber > block.super.ninodes) {
		return 0;
	}
	inode_bitmap.bits[inumber] = 0;

	char empty_buffer[DISK_BLOCK_SIZE] = {"\0"};
	int inode_block = inumber / INODES_PER_BLOCK + 1;
	int inode_index = inumber % INODES_PER_BLOCK;

	disk_read(inode_block, block.data);
	if (block.inode[inode_index].isvalid == 0) {
		return 0;
	}
	block.inode[inode_index].isvalid = 0;
	block.inode[inode_index].size = 0;
	int x;
	for (x = 0; x < POINTERS_PER_INODE; x++) {
		if (block.inode[inode_index].direct[x] != 0) {
			disk_write(block.inode[inode_index].direct[x], empty_buffer);
			block_bitmap.bits[block.inode[inode_index].direct[x]] = 0;
			block.inode[inode_index].direct[x] = 0;
		} else {
			break;
		}
	}

	if (block.inode[inode_index].indirect != 0) {
		block_bitmap.bits[block.inode[inode_index].indirect] = 0;
		int y;
		for (y = 0; y < POINTERS_PER_BLOCK; y++) {
			disk_read(block.inode[inode_index].indirect, block2.data);
			if (block2.pointers[y] != 0) {
				block_bitmap.bits[block2.pointers[y]] = 0;
			} else {
				break;
			}
		}
		disk_write(block.inode[inode_index].indirect, empty_buffer);
	}
	block.inode[inode_index].indirect = 0;
	disk_write(inode_block, block.data);

	return 1;
}

int fs_getsize( int inumber )
{
	union fs_block block, block1;

	disk_read(0,block1.data);

	int inode_block = inumber / INODES_PER_BLOCK + 1;
	int inode_index = inumber % INODES_PER_BLOCK;



	if(inumber <= 0 || inumber > block1.super.ninodes){ // catch invalid arguments
		return -1;
	}

	disk_read(inode_block,block.data);
	if(block.inode[inode_index].isvalid){
		return block.inode[inode_index].size; // return inode size parameter if it was valid
	}
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	union fs_block block, block1, block2, block3;
	disk_read(0,block.data);

	if(inumber <=0 || inumber > block.super.ninodes || inumber == 0) { // catch invalid input
		return 0;
	}

	int inode_block = inumber / INODES_PER_BLOCK +1;
	int inode_index = inumber % INODES_PER_BLOCK;

	// transfer all data from block to buffer
	disk_read(inode_block,block1.data);
	int total_size = block1.inode[inode_index].size;
	if(block1.inode[inode_index].isvalid){
		int i,j, total = 0;
		for (i=0;i<POINTERS_PER_INODE;i++){
			if(block1.inode[inode_index].direct[i] == 0){
				break;
			}
			if(offset >= DISK_BLOCK_SIZE){
				total_size -= DISK_BLOCK_SIZE;
				offset -= DISK_BLOCK_SIZE;
				continue;
			}

			disk_read(block1.inode[inode_index].direct[i], block2.data);

			for(j=offset;j<total_size && j < DISK_BLOCK_SIZE;j++){
				if(total < length){
					data[total] = block2.data[j];
					total++;	
				}else{
					i = POINTERS_PER_INODE;
					break;
				}
			}
			total_size -= DISK_BLOCK_SIZE;
		} // extra looping for when indirect blocks are needed
		if(total < length && total < total_size){
			disk_read(block1.inode[inode_index].indirect,block3.data);
			for (i=0;i<POINTERS_PER_BLOCK;i++){
				if(block3.pointers[i] == 0){
					break;
				}
				if(offset >= DISK_BLOCK_SIZE){
					total_size -= DISK_BLOCK_SIZE;
					offset -= DISK_BLOCK_SIZE;
					continue;
				}

				disk_read(block3.pointers[i], block2.data);

				for(j=offset;j<total_size && j < DISK_BLOCK_SIZE;j++){
					if(total < length){
						data[total] = block2.data[j];
						total++;	
					}else{
						i = POINTERS_PER_INODE;
						break;
					}
				}
				total_size -= DISK_BLOCK_SIZE;
			}
		}
		return total;
	}else{
		return 0;
	}
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	int true_offset = offset;
	union fs_block block, block1, block2, block3;
	disk_read(0,block.data);

	if(inumber <=0 || inumber > block.super.ninodes){ // catch invalid input
		printf("error\n");
		return 0;
	}

	int inode_block = inumber / INODES_PER_BLOCK +1;
	int inode_index = inumber % INODES_PER_BLOCK;

	// transfer all data from buffer to blocks
	disk_read(inode_block,block1.data);
	if(block1.inode[inode_index].isvalid){
		int i,j,k, total = 0;
		for (i=0;i<POINTERS_PER_INODE;i++){
			int block_found =0;
			if(block1.inode[inode_index].direct[i] == 0){
				for(k=1;k<disk_size();k++){
					if(block_bitmap.bits[k] != 1){
						block1.inode[inode_index].direct[i] = k;
						block_bitmap.bits[k] = 1;
						block_found = 1;
						break;
					}
				}
				if(!block_found){
					return 0; // error return if no blocks are available
				}
			}
			if(offset >= DISK_BLOCK_SIZE){
				offset -= DISK_BLOCK_SIZE;
				continue;
			}

			disk_read(block1.inode[inode_index].direct[i], block2.data);
			for(j=offset;j<length && j < DISK_BLOCK_SIZE;j++){
				if(total < length){
					block2.data[j] = data[total];
					total++;
				}else{
					disk_write(block1.inode[inode_index].direct[i],block2.data);
					i = POINTERS_PER_INODE;
					break;
				}
			}
			if(i != POINTERS_PER_INODE){
				disk_write(block1.inode[inode_index].direct[i],block2.data);
			}
			if(total >= length){
				i = POINTERS_PER_INODE;
			}
		} // extra looping for if indirect blocks are needed
		if(total < length){
			int i,j,k, found_flag = 0;
			if(block1.inode[inode_index].indirect == 0){
				for(i =0;i<disk_size();i++){
					if(block_bitmap.bits[i] != 1){
						block1.inode[inode_index].indirect = i;
						block_bitmap.bits[i] =1;
						found_flag = 1;
						break;
					}
				}
				if(!found_flag){
					return 0; // error return for if no blocks are available
				}
			}
			disk_read(block1.inode[inode_index].indirect, block3.data);

			for (i=0;i<POINTERS_PER_BLOCK;i++){
				int block_found = 0;
				if(block3.pointers[i] <= 0 || block3.pointers[i] >= disk_size()){
					for(k=1;k<disk_size();k++){
						if(block_bitmap.bits[k] != 1){
							block3.pointers[i] = k;
							block_bitmap.bits[k] = 1;
							block_found = 1;
							break;
						}
					}
					if(!block_found){
						return 0;
					}
				}
				disk_read(block3.pointers[i],block2.data);

				if(offset >= DISK_BLOCK_SIZE){
					offset -= DISK_BLOCK_SIZE;
					continue;
				}

				for(j=offset;j<length && j < DISK_BLOCK_SIZE;j++){
					if(total < length){
						block2.data[j] = data[total];
						total++;
					}else{
						disk_write(block3.pointers[i],block2.data);
						block3.pointers[i+1] = 0;
						i = POINTERS_PER_BLOCK;
						break;
					}
				}
				if(i != POINTERS_PER_BLOCK){
					disk_write(block3.pointers[i],block2.data);
					block3.pointers[i+1] = 0;
				}
				if(total >= length){
					i = POINTERS_PER_BLOCK;
				}
			}
			disk_write(block1.inode[inode_index].indirect,block3.data);
		}
		block1.inode[inode_index].size = total + true_offset;
		disk_write(inode_block,block1.data);
		return total;
	}else{
		return 0;
	}
	return 0;
}
