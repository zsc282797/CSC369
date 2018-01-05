#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "ext2.h"

#define MASK 7  //for masking out the mode bits 

//These two values are the same from EX7,8,9
unsigned char* disk;
int errors = 0; //this is used for traking for testing 
//This function used recursively would produce the name 
//of the directory at each level 


int find_next_level(char* path, char* dir, int st_inx){
	if (path[st_inx]!='/') {return -1; 
		//if this is not a valid absolute path
	}
	else { // if path starts with / as abs path
		int i;
		for (i=st_inx+1; path[i]!='/' && path[i]!='\0'; i++ ){ 
		//search for /'s position and copy name over 
			dir[i-st_inx-1] = path[i];
		}
		dir[i-st_inx-1] ='\0'; 
  		return i;
	}

}

//This function works with the above helper function 
//Given a path, return the inode number if found and 
//-ENOENT if not found 
int find_inode_for_path(char* path, unsigned char *disk){
	int inode_idx;  
	int block_idx;
  int name_idx = 0;
	struct ext2_inode* inode;
  struct ext2_dir_entry* dir;

    char* name = (char* )malloc(sizeof(char) * EXT2_NAME_LEN);
    //Prepare data for structures
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
    int inode_size = sb->s_inode_size;
    void* inode_table = disk + gc->bg_inode_table * 1024; 
    inode_idx = EXT2_ROOT_INO;//start from the root node 

    if (strlen(path)==3 &&strncmp(path,"/./",3)==0 ){
      return 1;
    }


  	//printf("indoe count = %d\n",sb->s_inodes_count);
  	while (1) {

    inode = (struct ext2_inode*) (inode_table + (inode_idx - 1) * sizeof(struct ext2_inode));
    int total_size = inode->i_size;
    
    if (name_idx == strlen(path)) {
      return inode_idx - 1;
    } //finished search for all the levels 

    name_idx = find_next_level(path, name,name_idx);//Search down for one level
    
    int exceed = 0;
    int new_inode_idx = -1;
    int block_num = 0;
    block_idx = inode->i_block[block_num];
    dir = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * block_idx);
    while (exceed < total_size) {
      exceed += dir->rec_len;
      if (dir->file_type == EXT2_FT_DIR) {
        if (strncmp(name, dir->name, dir->name_len) == 0) {
          
          new_inode_idx = dir->inode;
          break; //set the found inode as the next search root 
        }
      }
      else if (dir->file_type == EXT2_FT_REG_FILE) {
      
        //This shows that the name may contain garbage 
        if (strncmp(name, dir->name, dir->name_len) == 0) {
            new_inode_idx = dir->inode;
            break;
          }
        
      } //same idea as EX9 where we look for all the directory entries 
      dir = (void*)dir + dir->rec_len;
      if (exceed % EXT2_BLOCK_SIZE == 0) { //if exhast the block 
        block_num++; //move to the next block
        block_idx = inode->i_block[block_num];
        dir = (struct ext2_dir_entry*)(disk + EXT2_BLOCK_SIZE * block_idx);
      }
    }

    if (new_inode_idx < 0) {
      //If the new_inode_idx was never changed 
      return -2; //return not found error number
    } else {
      inode_idx = new_inode_idx;
    }

  }
}

  
char* get_parent_path(const char* path){


  int end=0;
  char * result = malloc(sizeof(char) * (strlen(path) + 1) );
  if (strncmp(path,"/./",3) == 0){
      strcpy(result,"/./");
      return result;
  }
  strcpy(result,path);
  for (end = strlen(result); end>=0; end--){
    if (result[end]=='/') break;
  }
 
  if (end == 0) return "/";
  else {
    result[end]='\0';
    return result;
  }
}
//this function returns the idx of the inode index starting from 0
//for the first free inode and if not found return -1 
int find_inode_bm(){

  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
  struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
  int i; int j;
  for (i = 0; i < 4; i++){
      unsigned int bb = *(disk+ gc->bg_inode_bitmap * 1024 +i);
     for (j = 0; j < 8; j++){
      
        if ( ((bb>>j)&1) == 0){
          printf("Free Inode got is: %d", i*8 + j);
          return i*8 + j;
        }
      }
  }
  return -1;
}
//This is a useful helper to flatten the bitmap into an array 
//for easyier search for free blocks
unsigned * block_bm2array(){
  struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
  unsigned * blocks = (unsigned *) malloc(sizeof(unsigned *) * sb->s_blocks_count);
  int i; int j;
  for (i = 0; i<16;i++){
    unsigned int bb = *(disk+ gc->bg_block_bitmap * 1024 +i);
    for (j=0;j<8;j++){
      blocks[i*8+j] = (bb>>j)&1;
    }
  }
  return blocks; 
}
//This helper function returns a array of indices to the free blocks
//if not enough blocks are found, return NULL
int* find_free_blocks(int size){
  struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
  int * blocks = (int* )malloc(sizeof(int)*(size+1));
  unsigned block_bits = sb->s_blocks_count;
  unsigned * b_bm = block_bm2array();
  int i; int found = 0;

  for (i = 0; i < block_bits; i++){
    if (b_bm[i]==0){
      printf("Found block index :%d\n",i);
      blocks[found] = i;
      found++;
    }
    if (found == size){return blocks; }
  }
  return NULL;
}
//This function padd the size of a struct 
int padd_struct(int size){
  if (size%4 == 0) 
    return size;
  else 
    return ((size / 4) +1)*4; 
}

void mask_block_bit(int block_idx){
  struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
  unsigned char * b_byte = (unsigned char*)(disk+ gc->bg_block_bitmap * 1024 + block_idx / 8);
  
  *b_byte = *b_byte | (1 << (block_idx % 8)); 
  
}

void mask_inode_bit(int inode_idx){
  struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
  struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
  unsigned char * i_byte = (unsigned char*)(disk+ gc->bg_inode_bitmap * 1024 + inode_idx/ 8);

  *i_byte = *i_byte | (1 << (inode_idx % 8)); 
}

char* get_actual_name(char* path){
  char* result = malloc(sizeof(char)*strlen(path));
  char* p_name = get_parent_path(path);
  
  int p_len = strlen(p_name);
  if (path[p_len]!='/'){
  strncpy(result,&path[p_len],strlen(path)-p_len);
  }
  else {
    strncpy(result,&path[p_len+1],strlen(path)-p_len);
  }
  return result;
}



//this function check if a file exist already in a directory 
//return 0 if not and return i_node idx if found 
int check_file_existence(struct ext2_inode *p_inode, char* file_name){
  int dir_blocks_count = p_inode->i_size / EXT2_BLOCK_SIZE; //get how many blocks this inode takes
  int block_idx; int i;
  struct ext2_dir_entry * dir_ent = NULL;
  for (i = 0; i< dir_blocks_count; i++){ //Walk through all the blocks 
    block_idx = p_inode -> i_block[i];
    dir_ent = (struct ext2_dir_entry*) (disk + EXT2_BLOCK_SIZE * block_idx);
    int exceed = 0;
    while (exceed < EXT2_BLOCK_SIZE){
      exceed += dir_ent -> rec_len; //track if we exahust the whole block 
      if ( dir_ent->file_type == EXT2_FT_REG_FILE 
        && strcmp(dir_ent->name,file_name)==0 ) { //if there is a match of the file name 
          return dir_ent->inode - 1;
      }
      dir_ent = (struct ext2_dir_entry *) ((void* ) dir_ent + dir_ent -> rec_len); //Going to the next entry 
    }
  }
  return 0;
}

//Check if direntry exist under p_inode and return it if does 
struct ext2_dir_entry* get_child_dirent(struct ext2_inode *p_inode, char* file_name){
  int dir_blocks_count = p_inode->i_size / EXT2_BLOCK_SIZE; //get how many blocks this inode takes
  int block_idx; int i;

  struct ext2_dir_entry * dir_ent = NULL;
  for (i = 0; i< dir_blocks_count; i++){ //Walk through all the blocks 
    block_idx = p_inode -> i_block[i];
    dir_ent = (struct ext2_dir_entry*) (disk + EXT2_BLOCK_SIZE * block_idx);
    int exceed = 0;
    while (exceed < EXT2_BLOCK_SIZE){
      exceed += dir_ent -> rec_len; //track if we exahust the whole block 
      if ( dir_ent->file_type == EXT2_FT_REG_FILE 
        && strcmp(dir_ent->name,file_name)==0 ) { //if there is a match of the file name 
          return dir_ent;
      }
      dir_ent = (struct ext2_dir_entry *) ((void* ) dir_ent + dir_ent -> rec_len); //Going to the next entry 
    }
  }
  return NULL;
}

void unmask_block_bit(int block_idx){
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    unsigned char * b_byte = (unsigned char*)(disk+ gc->bg_block_bitmap * 1024 + block_idx / 8);
  
    *b_byte = *b_byte & ~(1 << (block_idx % 8) ) ; 
}

void unmask_inode_bit(int inode_idx){
     struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    unsigned char * i_byte = (unsigned char*)(disk+ gc->bg_inode_bitmap * 1024 + inode_idx/ 8);
  
    *i_byte = *i_byte & ~(1 << (inode_idx % 8) ) ; 
}

int check_block_bit(int block_idx){
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    unsigned char * b_byte = (unsigned char*)(disk+ gc->bg_block_bitmap * 1024 + block_idx / 8);

    return (*b_byte & 1 << (block_idx % 8));
}

int check_inode_bit(int inode_idx){
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    unsigned char * i_byte = (unsigned char*)(disk+ gc->bg_inode_bitmap * 1024 + inode_idx/ 8);

    return (*i_byte & 1 << (inode_idx % 8));
}

int check_dirty_blocks(int node_idx) {

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
    void* inode_table = disk + EXT2_BLOCK_SIZE* gc->bg_inode_table;

    struct ext2_inode * inode = (struct ext2_inode *)(inode_table + sizeof(struct ext2_inode) * node_idx );
    int i = 0;
    int ind_count = inode->i_size / EXT2_BLOCK_SIZE - 12;  
    /*------------ Check the direct blocks if they are used by other operations ------------------*/
    for (i=0; i<12 ; i++){  
      if (inode->i_block[i]!=0){
        if (check_block_bit(inode->i_block[i])){
          return 1;
        }
      }
    }

    int * single_table = (int*)(disk + inode->i_block[12] * EXT2_BLOCK_SIZE); //Get the indirect table 
    for (i =0; i<ind_count; i++) {
        if (check_block_bit(single_table[i])){
          return 1;
        }
    }
  return 0;
}
void reset_blocks(int node_idx) {
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
    void* inode_table = disk + EXT2_BLOCK_SIZE* gc->bg_inode_table;

    struct ext2_inode * inode = (struct ext2_inode *)(inode_table + sizeof(struct ext2_inode) * node_idx );
    int i = 0;
    int ind_count = inode->i_size / EXT2_BLOCK_SIZE - 12;  
    /*------------ Check the direct blocks if they are used by other operations ------------------*/
    for (i=0; i<12 ; i++){  
      if (inode->i_block[i]!=0){
        unmask_block_bit(inode->i_block[i]);
        sb->s_free_blocks_count--;
        gc->bg_free_blocks_count--;
      }
    }

    int * single_table = (int*)(disk + inode->i_block[12] * EXT2_BLOCK_SIZE); //Get the indirect table 
    for (i =0; i<ind_count; i++) {
        unmask_block_bit(single_table[i]);
        sb->s_free_blocks_count--;
        gc->bg_free_blocks_count--;
    }
  return ;
}
//This helper checkes the inode counter to match bitmap 
//and the blocks counter to match bitmap too 

void check_counters(){
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
    unsigned char* b_map = (unsigned char*)(disk+ gc->bg_block_bitmap * EXT2_BLOCK_SIZE);
    unsigned char* i_map = (unsigned char*)(disk+ gc->bg_inode_bitmap * EXT2_BLOCK_SIZE);
    
    int byte; int offset; int block_free = 0; int inode_free = 0;
    /*---------- Test for block count for both superblock and groupblock-------------*/
    for (byte = 0; byte < (sb->s_blocks_count / 8); byte++) {
        for (offset = 0; offset < 8; offset++) {
            if ( (b_map[byte] & (1 << offset))==0 ) {
                block_free++;
            }
        }
    }
   if (block_free != sb->s_free_blocks_count) {
        unsigned diff = sb->s_free_blocks_count- block_free;
        printf("Fixed: superblock's free blocks counter was off by %d compared to the bitmap\n", abs(diff));
        errors += abs(diff);
        sb->s_free_blocks_count = block_free;
    }
    if (block_free != gc->bg_free_blocks_count) {
        unsigned diff = gc->bg_free_blocks_count - block_free ;
        printf("Fixed: block group's free blocks counter was off by %d compared to the bitmap\n", abs(diff));
        errors += abs(diff);
        gc->bg_free_blocks_count = block_free;
    }

    /*-------- Test for inode count for both superblock and groupblock ------------*/
    for (byte = 0; byte < (sb->s_inodes_count / 8); byte++) {
        for (offset = 0; offset < 8; offset++) {
            if ( (i_map[byte] & (1 << offset))==0 ) {
                inode_free++;
            }
        }
    }
    if (inode_free != sb->s_free_inodes_count){
        unsigned diff = inode_free - sb->s_free_inodes_count;
        printf("Fixed: superblock's free inodes counter was off by %d compard to the bitmap\n",abs(diff));
        errors+=abs(diff);
        sb->s_free_inodes_count = inode_free;
    }
     if (inode_free != gc->bg_free_inodes_count){
        unsigned diff = inode_free - gc->bg_free_inodes_count;
        printf("Fixed: superblock's free inodes counter was off by %d compard to the bitmap\n",abs(diff));
        errors+=abs(diff);
        gc->bg_free_inodes_count = inode_free;
    }


    return ;

}

void check_types(struct ext2_inode *inode) {
    int exceed = 0;
    struct ext2_dir_entry *dir_ent;
    struct ext2_inode* test_inode;
    struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
    void* inode_table = disk + EXT2_BLOCK_SIZE* gc->bg_inode_table;

    int i = 0;
    for (i=0; i<12 && inode->i_block[i]!=0 ;i ++ ){
        while (exceed < EXT2_BLOCK_SIZE) {
            dir_ent = (struct ext2_dir_entry *)(disk + (inode->i_block[i]) * EXT2_BLOCK_SIZE + exceed);
            test_inode = (struct ext2_inode *)(inode_table + (dir_ent->inode)*sizeof(struct ext2_inode));
            if (  S_ISLNK(test_inode->i_mode)   ) {
                if ( (dir_ent->file_type & MASK ) != EXT2_FT_SYMLINK ) { //If a symlink mismatch 
                    printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", dir_ent->inode);
                    dir_ent->file_type = EXT2_FT_SYMLINK; //Set the directory entry type to symlink 
                    errors++;
                }
            } else if ( S_ISREG(test_inode->i_mode) ) {
                if ( (dir_ent->file_type & MASK) != EXT2_FT_REG_FILE ) { // if a regular file 
                      
                    printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", dir_ent->inode);
                    dir_ent->file_type = EXT2_FT_REG_FILE;
                    errors++;
                }
            } else if ( S_ISDIR(test_inode->i_mode) ) {
                if ( (dir_ent->file_type & MASK) != EXT2_FT_DIR) {  // if a directory 
                    printf("Fixed: Entry type vs inode mismatch: inode [%d]\n", dir_ent->inode);
                    dir_ent->file_type = EXT2_FT_DIR;
                    errors++;
                }

                // check if dir_entry is . or .. before recursing
                if ( strncmp(dir_ent->name,".",1) == 0 || strncmp(dir_ent->name,"..",2) == 0) {
                    // if we are at a traversable dirent 
                    check_types(test_inode); //go to the subdirectory recursivly
                }
            } else {
                fprintf(stderr, "Type Error!\n");
            }
            exceed += dir_ent->rec_len; // move to the next rec_entry 
        }

        
    }

    return;
}

void check_inode_mapping(){
   int i =0;
   struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
   struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
   for (i=0; i<sb->s_inodes_count; i++){
        struct ext2_inode * ei = (struct ext2_inode *)(disk + gc->bg_inode_table*1024+i*sb->s_inode_size);

        if ( S_ISREG(ei->i_mode) || S_ISDIR(ei->i_mode) || S_ISLNK(ei->i_mode) ){
            if ( !check_inode_bit(i) ){ // if inode is not marked 
                printf("Fixed: inode [%d] not marked as in-use\n",i+1);
                mask_inode_bit(i);
                errors ++;
            }
        }
    }
}

void check_block_mapping(){
   int i =0; int j=0; int k=0;
   struct ext2_group_desc *gc = (struct ext2_group_desc *)(disk+EXT2_BLOCK_SIZE*2);
   struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
   for (i=0; i<sb->s_inodes_count; i++){ //Go through all the inodes 
      int fixed = 0; //error count for each node 
      struct ext2_inode * ei = (struct ext2_inode *)(disk + gc->bg_inode_table*1024+i*sb->s_inode_size);
      for (j=0; j<12 && ei->i_block[j]!=0; j++){ //Go through all the blocks of the inode 

          if( !check_block_bit (ei->i_block[j]-1) ){ //if block is not marked 
              mask_block_bit(ei->i_block[j]-1);
              fixed ++; errors ++;
          }
      }
      //indirect block
      if (ei->i_block[12] != 0){ //if indirection is found 
        int ind_count = ei->i_size/EXT2_BLOCK_SIZE +1 - 12;   
        int * single_table = (int *)(disk + ei->i_block[12] * EXT2_BLOCK_SIZE );
        for (k=0; k<ind_count; k++){
          if( !check_block_bit (single_table[k]) ){
            mask_block_bit(single_table[k]);
            fixed ++; errors ++;
          }
        }
      }
      if (fixed > 0){
      printf("Fixed: %d in-use data blocks not marked in data bitmap for inode: [%d]\n",fixed,i+1);}
   }
}
int get_error(){
  return errors;
}
