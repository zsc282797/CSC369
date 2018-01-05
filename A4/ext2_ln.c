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
	int block_used = 0;  int symlink = 0; //This keeps track whether the link is symbolic   
	int inode_used = 0;  
	/*---------------  Handling of inputs -----------------*/
	if (argc != 4 && argc !=5) {
    	fprintf(stderr, "Usage: ext2_ln <image file name> <source file path> <linking path> -s(optional for symbolic)\n");
		exit(EINVAL);
    }
    if (argc == 5){symlink = 1;} //set the symbolic to be true 
    char* src_path = malloc(sizeof(char) * (strlen(argv[2])+3) );
    char* link_path = malloc(sizeof(char) * (strlen(argv[3])+3) );


    strcpy(src_path, "/.");  strcpy(link_path, "/.");
    strcat(src_path,argv[2]); strcat(link_path, argv[3]);
    char* src_p_path = get_parent_path(src_path);
    char* link_p_path = get_parent_path(link_path);


    
    char* src_name = get_actual_name(src_path);
    char* link_name = get_actual_name(link_path);

    //copy over the paths 
    
  	if (strlen(src_path)>1 && src_path[strlen(src_path)-1]=='/'){
  		src_path[strlen(src_path)-1]='\0'; // take out the last / if not root 
  		
  	}
  	if (strlen(link_path)>1 && link_path[strlen(link_path)-1]=='/'){
  		link_path[strlen(link_path)-1]='\0'; // take out the last / if not root 
  		
  	}

  	/*--------------- Operation on input file ---------------*/
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
    /*--------------------- Sanity Checks and prep ----------------------*/
    int src_p_inode_idx = find_inode_for_path(src_p_path, disk);
    int link_p_inode_idx = find_inode_for_path(link_p_path, disk);
    struct ext2_inode* src_p_inode = (struct ext2_inode *) (inode_table + src_p_inode_idx * sizeof(struct ext2_inode));
    struct ext2_inode* link_p_inode = (struct ext2_inode *) (inode_table + link_p_inode_idx * sizeof(struct ext2_inode));
    if (src_p_inode_idx < 0) {
    	fprintf(stderr,"No path find for source \n");
    	exit(ENOENT);
    }
    if (link_p_inode_idx < 0) {
    	fprintf(stderr,"No path find for link  \n");
    	exit(ENOENT);
    }

    // Check if des path already exist 
    int link_idx = check_file_existence(link_p_inode, link_name);
    if ( link_idx > 0 ){ 
    	fprintf(stderr, "%s already exist as source", link_path);
    	exit(EEXIST);
    }
    int src_idx = find_inode_for_path(src_path,disk);

    if ( src_idx < 0 ){
    	fprintf(stderr, "No such source exist as %s", src_path);
    	exit(ENOENT);
	} 
	struct ext2_inode* src_inode = (struct ext2_inode *) (inode_table + src_idx * sizeof(struct ext2_inode));
	if (src_inode -> i_mode == EXT2_S_IFDIR && !symlink ){
		fprintf(stderr,"Cannot create hardlink for directory!");
		exit(EINVAL);
	}	
	struct ext2_inode * sym_inode;
	int sym_inode_idx;
	/*----------------------- Create an inode for Symbolic ----------------*/
	if (symlink){
		printf("Creating symbolic link");
		sym_inode_idx = find_inode_bm();
		if (sym_inode_idx < 0){
			fprintf(stderr, "Cannot allocate inode!\n");
			exit(ENOSPC);
		}
		sym_inode = (struct ext2_inode *)(inode_table + sym_inode_idx * sizeof(struct ext2_inode));
		//copy over the items and setup the sym_inode 
		sym_inode -> i_mode = EXT2_S_IFLNK;
		sym_inode -> i_size = src_inode -> i_size;
		sym_inode -> i_links_count = 1;
		int s_i;
		for (s_i = 0; s_i < src_inode ->i_blocks; s_i++){
			sym_inode->i_block[s_i] = src_inode->i_block[s_i]; //copy over the blocks 
		}

		mask_inode_bit(sym_inode_idx);
    }
    /*----------------------- Add the file entry to the parent directory entry ------------*/
    //(Possible good idea for a helper function ...EM....We will see )
    //get the actual dir name and its length 
    struct ext2_inode * p_dir_inode = link_p_inode; //Set for reusing code 
    int dir_block_count = p_dir_inode->i_size / EXT2_BLOCK_SIZE;

    int len_name = strlen(link_name);
    int new_ent_size = padd_struct(sizeof(struct ext2_dir_entry) + len_name);
    int found = 0; int i;
    struct ext2_dir_entry * dir_ent;
    for (i=0;i<dir_block_count;i++){
      dir_ent = (struct ext2_dir_entry *)(disk + p_dir_inode->i_block[i]*EXT2_BLOCK_SIZE);
      int curr_size = 0;
      while (curr_size < EXT2_BLOCK_SIZE) { //walk throgh all the entries
        curr_size += dir_ent->rec_len;
        if (curr_size == EXT2_BLOCK_SIZE && dir_ent->rec_len >= new_ent_size + padd_struct(sizeof(struct ext2_dir_entry)+dir_ent->name_len ) ){
          //Based on the fact that the last dir_entry will exhaust the block 
          found = 1;
            break;
        }
        dir_ent = (struct ext2_dir_entry *)( (void*) dir_ent + dir_ent->rec_len );
      }
      if (found) break;
    }
  if (!found){//if no such space exist in the blocks table 
    int* free_blk = find_free_blocks(1); //allocate a new block 
    if (free_blk == NULL) {
      fprintf(stderr,"Cannot allocate more blocks\n");
      exit(ENOSPC); 
    }
    printf("Allocate a new block");
    dir_ent =(struct ext2_dir_entry *)(disk + (free_blk[0]+1) *EXT2_BLOCK_SIZE);
    if (!symlink){
    dir_ent->inode = src_idx + 1;
    dir_ent->rec_len = new_ent_size;
    dir_ent->name_len = len_name;
    dir_ent->file_type = EXT2_FT_REG_FILE;
    strcpy(dir_ent->name,link_name);
    mask_block_bit(free_blk[0]); block_used++;
	}
	else { //if it is a symlink 
		dir_ent->inode = sym_inode_idx + 1;
		dir_ent->rec_len = new_ent_size;
    	dir_ent->name_len = len_name;
    	strcpy(dir_ent->name,link_name);
    	dir_ent->file_type = EXT2_FT_SYMLINK;
    	mask_block_bit(free_blk[0]); block_used++;
	}	
    p_dir_inode->i_size += EXT2_BLOCK_SIZE;
    p_dir_inode->i_blocks += 2;
    p_dir_inode->i_block[dir_block_count] = free_blk[0]+1;

  }
  else {//if such spaces is found 

    int old_len = dir_ent->rec_len;

    int new_len = padd_struct( sizeof(struct ext2_dir_entry) + dir_ent->name_len );
    dir_ent->rec_len = new_len;

    dir_ent = (struct ext2_dir_entry*) ((void*) dir_ent + dir_ent->rec_len);
    if (!symlink){
    dir_ent->inode = src_idx + 1;
    dir_ent->file_type = EXT2_FT_REG_FILE;
    dir_ent->name_len = len_name;
    strcpy(dir_ent->name,link_name);
    dir_ent->rec_len = old_len - new_len;
	}
	else { //if it is a symbolic link 
		 dir_ent->inode = sym_inode_idx + 1;
    	dir_ent->file_type = EXT2_FT_SYMLINK;
    	dir_ent->name_len = len_name;
    	strcpy(dir_ent->name,link_name);
    	dir_ent->rec_len = old_len - new_len; //So we make sure that we jump to the next block 

	}

  
  } 

   //struct ext2_inode* src_inode = (struct ext2_inode*) (inode_table+ src_idx * sizeof(struct ext2_inode) );
   src_inode -> i_links_count ++;  
   
   gc->bg_free_blocks_count -= block_used;
   sb->s_free_blocks_count -= block_used;
   gc->bg_free_inodes_count -= inode_used;
   sb->s_inodes_count -= inode_used;	
   return 0;


}