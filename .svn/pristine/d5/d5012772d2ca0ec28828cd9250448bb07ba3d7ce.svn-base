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
	/*--------------- Book keeping variables---------------*/
	int block_used = 0;  int in_direct = 0; //this keeps if the file needs in_direct entries  
	int inode_used = 0;  int blks_required; //this stores the blokcs required 
	/*---------------  Handling of inputs -----------------*/
	if (argc != 4) {
    	fprintf(stderr, "Usage: ext2_cp <image file name> <source file path> <destination path>\n");
		exit(1);
  }
  /*--------------- Operation on input file ---------------*/
  int img_fd = open(argv[1], O_RDWR); //readin the image file for the disk 
  int input_fd = open(argv[2], O_RDONLY); //readin the input file for reading 
  if (input_fd < 0){
    fprintf(stderr,"Cannot read source file %s\n",argv[2]);
    exit(ENOENT);
  }
  //Get the size and number of blocks for the file 
  FILE* in_fp = fopen(argv[2],"rb");
  if (in_fp == NULL){
    fprintf(stderr,"Cannot read source file %s\n",argv[2]);
    exit(ENOENT);
  }
  fseek(in_fp, 0L, SEEK_END);
  int input_size = ftell(in_fp); 
  rewind(in_fp);
  blks_required = input_size / EXT2_BLOCK_SIZE; // GET the blocks required 
  if (input_size % EXT2_BLOCK_SIZE != 0){
    blks_required++;
  }
  
  if (blks_required > 12) { //since we have 12 direct pointers to blocks 
      in_direct = 1; //set the in_direct to true 
      blks_required += 1; //one links to the indirect table 
  }
  /*--------------- Operation on paths ---------------------*/
  char * path = malloc(sizeof(char)* (strlen(argv[3])+3) ); 
  //char * p_path =  malloc(sizeof(char)* (strlen(argv[3])+1) ); 
  char * fpath = malloc(sizeof(char)* (strlen(argv[2])+3) ); 
  strcpy(path, argv[3]); //copy in the full path for destination 
  strcpy(fpath, argv[2]); //copy in the full path for input_file 
  char * fname = get_actual_name(fpath);
  if (strlen(path)>1 && path[strlen(path)-1]=='/'){
      path[strlen(path)-1]='\0'; // take out the last / if not root 
  }
  char * new_path = malloc(sizeof(char)* (strlen(argv[3])+3) );
  strcpy(new_path,"/.");
  strcat(new_path, path);
  path = new_path;
  /*------------ DEBUG print outs --------------*/
  printf("Destination path : %s \n", path);
  printf("File name to be moved: %s\n",fname);
   

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
  /* -------- Intergrity check before operation --------------*/
    int pinode_idx = find_inode_for_path(path,disk); //find the inode number for the dir to put the file 
    if (pinode_idx < 0) { //did not find the parent dir 
      fprintf(stderr,"%s doesn not exist \n",path);
      exit(ENOENT);
    }
    struct ext2_inode * p_inode = (struct ext2_inode *) (inode_table + pinode_idx * sizeof(struct ext2_inode)); 
    //Grab the actual inode struct 
    int exist_file = check_file_existence(p_inode, fname); //test if file already exist 
    if (exist_file >= 1) { 
      fprintf(stderr,"File %s already exixst in directory %s \n",fname, path);
      exit(EEXIST);
    }
  /* ----------- File operations ---------------------*/
    //Grab the free blocks 
    int* free_blks = find_free_blocks(blks_required);
    if (free_blks == NULL) { // if failed to get free blocks
        fprintf(stderr, "Not enough blocks for the file\n");
        exit(ENOSPC);
    }
    block_used += blks_required;
    
    //Map the files to the memory (This may overflow the memory if file is large! )
    unsigned char * input_file_map = mmap(NULL, input_size, PROT_READ | PROT_WRITE,MAP_PRIVATE, input_fd, 0);
    int to_copy = input_size; //Trace how many bytes left to be copied over 
    int copied = 0; // Trace how many bytes are copied over 
    int copy_count; // Trace how many blks to be copied to the blocks 
    copy_count = blks_required; 
    //start copy per block
    int c_idx; //copy index 
    for (c_idx = 0; c_idx< copy_count;c_idx++){
        if (to_copy < EXT2_BLOCK_SIZE ){ //if the left over is not enough for a full block 
            memcpy(disk + EXT2_BLOCK_SIZE * (free_blks[c_idx]+1), input_file_map + copied, to_copy); //only copy over what is left 
            to_copy =0; //mark finished 
        }
        else { //copy by block size (1024 )
            memcpy(disk + EXT2_BLOCK_SIZE * (free_blks[c_idx]+1), input_file_map + copied, EXT2_BLOCK_SIZE); 
            to_copy -= EXT2_BLOCK_SIZE; //one less block to copy 
            copied += EXT2_BLOCK_SIZE;
        }
        mask_block_bit(free_blks[c_idx]); //mask the bit as used;
    } 
    int single_table;
    //IF SINGLE INDIRECT IS REQUIRED 
    if (in_direct){
      single_table = free_blks[blks_required-1]; //Get the last free blk as the table block 
      mask_block_bit(single_table); //Mark this as used 
      int * entries = (int*)(disk + EXT2_BLOCK_SIZE * (single_table + 1)); //get the first entry place 
      int indirect_idx; //index of the indirect blocks pointer 
      for(indirect_idx=12  ; indirect_idx< blks_required-1; indirect_idx++){//stores the link the the blocks 
          * entries = free_blks[indirect_idx] + 1; 
          entries ++; //next entry 
      } 
    } //Now we have the indirect table stored at single_table (indx of block )

    /* -------------------- Set up inode for the file ---------------------- */ 
    int f_inode_idx = find_inode_bm(); //get free inode from the bitmap 
    if (f_inode_idx < 0){ //if no inode is found 
      fprintf(stderr,"No inode can be reserved for file\n");
      exit(ENOSPC);
    }
    struct ext2_inode * f_inode = (struct ext2_inode *)(inode_table + f_inode_idx * sizeof(struct ext2_inode) );
    f_inode -> i_mode = EXT2_S_IFREG;
    f_inode -> i_size = input_size;
    f_inode -> i_links_count = 1; //since this is a file
    f_inode -> i_blocks = blks_required * 2; //Thanks for the notes ,disk sectors are 512 bytes each 
    int blk_i; //represent blk index 
    for (blk_i=0; blk_i < blks_required && blk_i < 11; blk_i++){ //populating the direct table 
        f_inode->i_block[blk_i] = free_blks[blk_i] + 1;
    }
    if (in_direct){
        f_inode -> i_block[12] = single_table; //set the single indirect table 
    }
    mask_inode_bit(f_inode_idx);
    inode_used++;


    /*----------------------- Add the file entry to the parent directory entry ------------*/
    //(Possible good idea for a helper function ...EM....We will see )
    //get the actual dir name and its length 
    struct ext2_inode * p_dir_inode = p_inode; //Set for reusing code 
    int dir_block_count = p_dir_inode->i_size / EXT2_BLOCK_SIZE;
   
    int len_name = strlen(fname);
    int new_ent_size = padd_struct(sizeof(struct ext2_dir_entry) + len_name);
    int found = 0; int i;
    struct ext2_dir_entry * dir_ent;
    for (i=0;i<dir_block_count;i++){
      dir_ent = (struct ext2_dir_entry *)(disk + p_dir_inode->i_block[i]*EXT2_BLOCK_SIZE);
      int curr_size = 0;
      while (curr_size < EXT2_BLOCK_SIZE) { //walk throgh all the entries
        curr_size += dir_ent->rec_len;
        if (curr_size == EXT2_BLOCK_SIZE && dir_ent->rec_len >= new_ent_size + padd_struct(sizeof(struct ext2_dir_entry)+dir_ent->name_len) ){
          //Based on the fact that the last dir_entry of the block will exhast the block 
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
  
    dir_ent =(struct ext2_dir_entry *)(disk + (free_blk[0]+1) *EXT2_BLOCK_SIZE);
    dir_ent->inode = f_inode_idx + 1;
    dir_ent->rec_len = new_ent_size;
    dir_ent->name_len = len_name;
    dir_ent->file_type = EXT2_FT_REG_FILE;
    strcpy(dir_ent->name,fname);
    mask_block_bit(free_blk[0]); block_used++;

    p_dir_inode->i_size += EXT2_BLOCK_SIZE;
    p_dir_inode->i_blocks += 2;
    p_dir_inode->i_block[dir_block_count] = free_blk[0]+1;

  }
  else {//if such spaces is found 

    int old_len = dir_ent->rec_len;
    
    int new_len = padd_struct( sizeof(struct ext2_dir_entry) + dir_ent->name_len );
    dir_ent->rec_len = new_len;

    dir_ent = (struct ext2_dir_entry*) ((void*) dir_ent + dir_ent->rec_len);
    dir_ent->inode = f_inode_idx + 1;
    dir_ent->file_type = EXT2_FT_REG_FILE;
    dir_ent->name_len = len_name;
    strcpy(dir_ent->name,fname);
    dir_ent->rec_len = old_len - new_len;
   
  } 

  /*------------------------ Book keeping for Group Desc---------------------*/
  gc->bg_free_blocks_count -= block_used;
  gc->bg_free_inodes_count -= inode_used;
  gc->bg_used_dirs_count++;

  return 0;


}