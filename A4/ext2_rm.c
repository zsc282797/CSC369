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
#include <getopt.h>

unsigned char* disk;

int main(int argc, char **argv){
/*--------------- Book keeping variables---------------*/
	int block_freed = 0;  int symlink = 0; //This keeps track whether the link is symbolic   
	int inode_freed = 0;  int hardlink = 0;// This keeps track if the dir_ent is a hard link 
/*---------------  Handling of inputs -----------------*/
	//This does not support the -r tag for recursive delete
	if (argc != 3) {
    	fprintf(stderr, "Usage: ext2_rm <image file name> <file path to delete>\n");
		exit(EINVAL);
    }
/*--------------- Readin Path to delete -----------------*/
    char* rm_path = malloc(sizeof(char)*(strlen(argv[2])+3));
    char * new_path = malloc(sizeof(char)* (strlen(argv[2])+3) );
    strcpy(rm_path,argv[2]);
     strcpy(new_path,"/.");
    strcat(new_path, rm_path);
    rm_path = new_path;


    
    char* rm_p_path = get_parent_path(rm_path);
    char* rm_name = get_actual_name(rm_path);

    
    if (strlen(rm_path)>1 && rm_path[strlen(rm_path)-1] == '/') {
		rm_path[strlen(rm_path)-1] = '\0';
  
    }

  
/*--------------- Loading image files -------------------*/
    	
  	int img_fd = open(argv[1], O_RDWR); //readin the image file for the disk 


  /* ----- Prepare the file system and get pointers to block ------- */
  	disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, img_fd, 0);
  	if (disk == MAP_FAILED) {
    	perror("mmap");
    	exit(1);
  	} //Code reused form Exercise starter code for reading disk img 
  	//Set up structs and pointers (Possible helper to reduce repeating ?)

	struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
    void* inode_table = disk + EXT2_BLOCK_SIZE* gc->bg_inode_table;

    //void* bitmap_inode = disk + EXT2_BLOCK_SIZE * gc->bg_inode_bitmap;
    void* bitmap_blcok = disk + EXT2_BLOCK_SIZE * gc->bg_block_bitmap;
    unsigned int inode_bitmap_offset = gc->bg_inode_bitmap;

  /* ---------- Checking for the rm path --------------*/ 
  	int file_inode_idx = find_inode_for_path(rm_path, disk);

  	if (file_inode_idx < 0) {// no inode found two situation can happen here, there is no file or it is a hardlink 
		//test if it is a hard link 

		
  		int rm_p_idx = find_inode_for_path(rm_p_path,disk);
  		if (rm_p_idx < 0) {
  			fprintf(stderr,"No file found to remove!\n"); exit(ENOENT);
	  	}
	  	struct ext2_inode * rm_p_inode = (struct ext2_inode*) (inode_table + sizeof(struct ext2_inode) * rm_p_idx );
	  	struct ext2_dir_entry* ln_blk = get_child_dirent(rm_p_inode, rm_name);
	  	if (ln_blk!=NULL){
	  		printf("Path is a hardlink\n");
	  	} 
	  	else {
	  		fprintf(stderr,"No file found to remove!\n"); exit(ENOENT);
	  	}
	} 
	else { //an inode is found 
		struct ext2_inode * rm_file_inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * file_inode_idx);
		if (S_ISLNK(rm_file_inode -> i_mode)){
			printf("Path is a symbolic link!\n");
		}
		else if (S_ISREG(rm_file_inode -> i_mode)){
			printf("Path is a regular file\n");
		}
		else if (S_ISDIR(rm_file_inode -> i_mode)){
			fprintf(stderr,"Cannot rm a direcotry: %s",rm_path);
			exit(EINVAL);
		}
		else {
			printf("WTF?!\n");
			printf("file_inode_idx = %d\n",file_inode_idx);
			printf("rm_file_inode mode = %x",rm_file_inode->i_mode);
		}

	}
	//Assume we are only delete a regular file
	//Call our helper 
	int rm_p_idx = find_inode_for_path(rm_p_path,disk);
    
	int exceed = 0;
	
	struct ext2_dir_entry* ent;
	struct ext2_dir_entry* prev; //This is used to "cover" the gap 
	struct ext2_inode * rm_p_inode = (struct ext2_inode*) (inode_table + sizeof(struct ext2_inode) * rm_p_idx );
	struct ext2_inode * rm_inode = (struct ext2_inode*) (inode_table + sizeof(struct ext2_inode) * file_inode_idx);
	char* name = get_actual_name(rm_path);

	int i;
	 for (i = 0; i < 12; i ++) {//Walking through all the blocks of the parent's inode 
        
	 		exceed = 0;
            if (rm_p_inode->i_block[i] != 0) { //walk through all the entries 
                ent = (struct ext2_dir_entry *)(disk + (rm_p_inode->i_block[i]) * EXT2_BLOCK_SIZE + exceed);
                
                if (ent->inode-1 == file_inode_idx){ //First entry of each block 
                
                	rm_inode->i_links_count --;
                	ent->inode = 0; 
               		if (rm_inode->i_links_count == 0) { //removing the file 
                        for (i =0; i<12; i++) { //for the first direct block, just mark all the block as usable  
                        	if (rm_inode->i_block[i]!=0){
                        		unmask_block_bit(rm_inode->i_block[i]);
                        		block_freed ++;  //we are going to add it back to group and super block 

                        	}
                        }
                        int * single_table = (int*)(disk+ rm_inode->i_block[12]* EXT2_BLOCK_SIZE );
                        int ind_blocks = (rm_inode->i_size)/EXT2_BLOCK_SIZE - 12; //calculated indirect blocks 
                        for (i = 0; i< ind_blocks; i++){
                        	unmask_block_bit(single_table[i]);
                        	block_freed ++; 
                        } 
                        unmask_inode_bit(file_inode_idx);
                        inode_freed ++;
                        rm_inode -> i_dtime = time(NULL); //from StackOverflow: http://stackoverflow.com/questions/5141960/get-the-current-time-in-c
                    }
                    
                }
				prev = ent;
                exceed += ent->rec_len; //Move on to the next entry 
				while (exceed < EXT2_BLOCK_SIZE) {
                    ent = (struct ext2_dir_entry *)(disk + (rm_p_inode->i_block[i]) * EXT2_BLOCK_SIZE + exceed);
                    if (ent->inode-1 == file_inode_idx) {
                   
                        prev->rec_len += ent->rec_len;
                        rm_inode->i_links_count --;
                       
                        if (rm_inode->i_links_count == 0) {
                            for (i =0; i<12; i++) { //for the first direct block, just mark all the block as usable  
                        	if (rm_inode->i_block[i]!=0){
                        		unmask_block_bit(rm_inode->i_block[i]);
                        		block_freed ++;  //we are going to add it back to group and super block 

                        	}
                        }
                        int * single_table = (int*)(disk+ rm_inode->i_block[12]* EXT2_BLOCK_SIZE );
                        int ind_blocks = (rm_inode->i_size)/EXT2_BLOCK_SIZE - 12; //calculated indirect blocks 
                        for (i = 0; i< ind_blocks; i++){
                        	unmask_block_bit(single_table[i]);
                        	block_freed ++; 
                        } 
                        unmask_inode_bit(file_inode_idx);
                        inode_freed ++;
                        rm_inode -> i_dtime = time(NULL); //from StackOverflow: http://stackoverflow.com/questions/5141960/get-the-current-time-in-c
                        }
                    }

                    exceed += ent->rec_len;
                    prev = ent;
                }
            }
        }

     sb->s_free_blocks_count += block_freed;
     gc->bg_free_blocks_count += block_freed;
     sb->s_free_inodes_count += inode_freed;
     gc->bg_free_inodes_count += inode_freed;

return 0;
}