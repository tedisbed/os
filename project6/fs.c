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

int fs_format()
{
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
				printf("inode %d:\n", (i - 1) * block.super.ninodeblocks + j);
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

				if (indirect_needed == 1) {
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
	return 0;
}

int fs_create()
{
	return 0;
}

int fs_delete( int inumber )
{
	return 0;
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
