#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

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
	int in_use;
};

struct fs_bitmap inode_bitmap = {.in_use = 0};
struct fs_bitmap block_bitmap = {.in_use = 0};

int fs_format()
{
	if (block_bitmap.in_use) {
		return 0;
	}

	union fs_block block;
	disk_read(0, block.data);
	block.super.magic = FS_MAGIC;
	block.super.nblocks = disk_size();
	block.super.ninodeblocks = block.super.nblocks / 10;
	if (block.super.nblocks % 10 != 0) {
		block.super.ninodeblocks++;
	}
	block.super.ninodes = block.super.ninodeblocks * INODES_PER_BLOCK;
	disk_write(0, block.data);

	int i;
	char buffer[DISK_BLOCK_SIZE] = {"\0"};
	for (i = 1; i < block.super.nblocks; i++) {
		disk_write(i, buffer);
	}

	return 1;
}

void fs_debug()
{
	union fs_block block, block2, block3;

	disk_read(0,block.data);

	printf("superblock:\n");
	if (block.super.magic == FS_MAGIC) {
		printf("    magic number is valid\n");
	} else {
		printf("    magic number is invalid\n");
	}
	printf("    %d blocks on disk\n",block.super.nblocks);
	printf("    %d blocks for inodes\n",block.super.ninodeblocks);
	printf("    %d inodes total\n",block.super.ninodes);

	int i, j;
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
	if (block.super.magic != FS_MAGIC || block_bitmap.in_use) {
		return 0;
	}

	inode_bitmap.bits = calloc(block.super.ninodeblocks * INODES_PER_BLOCK, sizeof(int));
	block_bitmap.bits = calloc(disk_size(), sizeof(int)); // NEED TO FREE THESE
	block_bitmap.in_use = 1;
	int i, j, k;
	for (i = 0; i < 1 + block.super.ninodeblocks; i++) {
		block_bitmap.bits[i] = 1;
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
	if (!block_bitmap.in_use) {
		return 0;
	}

	union fs_block block, block2;
	disk_read(0, block.data);

	int i, j;
	int inumber = 0;
	for (i = 1; i < disk_size(); i++) {
		if (block_bitmap.bits[i] == 0) {
			for (j = 1; j < block.super.ninodeblocks * INODES_PER_BLOCK; j++) {
				if (inode_bitmap.bits[j] == 0) {
					inode_bitmap.bits[j] = 1;
					inumber = j;
					break;
				}
			}
			if (inumber != 0) {
				block_bitmap.bits[i] = 1;
				int inode_index = inumber / INODES_PER_BLOCK + 1;
				disk_read(inode_index, block2.data);
				block2.inode[inumber % INODES_PER_BLOCK].isvalid = 1;
				block2.inode[inumber % INODES_PER_BLOCK].size = 0;
				block2.inode[inumber % INODES_PER_BLOCK].direct[0] = i;
				int x;
				for (x = 1; x < POINTERS_PER_INODE; x++) {
					block2.inode[inumber % INODES_PER_BLOCK].direct[x] = 0;
				}
				block2.inode[inumber % INODES_PER_BLOCK].indirect = 0;
				disk_write(inode_index, block2.data);
				break;
			}
		}
	}

	return inumber;
}

int fs_delete( int inumber )
{
	if (!block_bitmap.in_use) {
		return 0;
	}

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
			block.inode[inode_index].direct[x] = 0;
			block_bitmap.bits[block.inode[inode_index].direct[x]] = 0;
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
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	return 0;
}
