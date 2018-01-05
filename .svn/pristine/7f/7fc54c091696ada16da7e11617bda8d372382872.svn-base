#ifndef HELPER_LK
#define HELPER_LK

#include "ext2.h"

int find_next_level(char* path, char* dir, int st_inx);
int find_inode_for_path(char* path, unsigned char *disk);
char* get_parent_path(char* path);
int find_inode_bm();
unsigned * block_bm2array();
int* find_free_blocks(int size);
int padd_struct(int size);
void mask_block_bit(int block_idx);
char *get_actual_name(char* path);
void mask_inode_bit(int inode_idx);
int check_file_existence(struct ext2_inode *p_inode, char* file_name);
struct ext2_dir_entry* get_child_dirent(struct ext2_inode *p_inode, char* file_name);
void unmask_block_bit(int block_idx);
void unmask_inode_bit(int block_idx);
int check_block_bit(int block_idx);
int check_inode_bit(int inode_idx);
int check_dirty_blocks(int node_idx);
void reset_blocks(int node_idx);
void check_counters();
void check_types(struct ext2_inode *inode);
void check_inode_mapping();
void check_block_mapping();
int get_error();
#endif

