#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>

#include "ext2.h"
#include "helper.h"

unsigned char* disk;

int main(int argc, char **argv){
	 if (argc != 2) {
        fprintf(stderr, "Usage: ext2_checker <image file name>\n");
        exit(1);
    }
    /* ----- Prepare the file system and get pointers to block ------- */
    int img_fd = open(argv[1], O_RDWR); //readin the image file for the disk 
 	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, img_fd, 0);
  	if (disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
  	}
  	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
    void* inode_table = disk + EXT2_BLOCK_SIZE* gc->bg_inode_table;
	/*----------- Call helpers to do the job ----------------*/
  	struct ext2_inode * root = (struct ext2_inode*) (inode_table + EXT2_ROOT_INO * sizeof(struct ext2_inode) );
	check_counters();
	check_types(root); //starting from the root down 
	check_inode_mapping();
	check_block_mapping();
	check_counters();

	int total = get_error();
	if (total == 0){
		printf("No file system inconsistencies detected!\n");
	}
	else {
		printf("%d file system inconsistencies detected!\n",total);
	}

}
