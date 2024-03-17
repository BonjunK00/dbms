#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <vector>
#include "buffer.h"


// TYPES.

// Type is declared in "page.h".

typedef struct header_page_t header_node;
typedef struct leaf_page_t leaf_node;
typedef struct internal_page_t node;



// FUNCTION PROTOTYPES.

// Output and utility.

void print_bpt( int64_t table_id );
void print_leaves( int64_t table_id );
int find_range( int64_t table_id, int64_t begin_key, int64_t end_key, 
                std::vector<int64_t>* keys, std::vector<char*>* values,
                std::vector<uint16_t>* val_sizes);
pagenum_t find_leaf( int64_t table_id, int64_t key);
int find( int64_t table_id, int64_t key, char * ret_val, uint16_t * val_size);
int64_t read_leaf_key(leaf_node * leaf, int i);
uint16_t read_leaf_val_size(leaf_node * leaf, int i);
uint16_t read_leaf_offset(leaf_node * leaf, int i);
void read_leaf_value(leaf_node * leaf, char * ret_val, int i);
uint64_t read_temp_body_key(uint8_t * body, int i);
uint16_t read_temp_body_val_size(uint8_t * body, int i);
uint16_t read_temp_body_offset(uint8_t * body, int i);
void read_temp_body_value(uint8_t * body, char * ret_val, int i);


// Insertion.

void write_leaf_key(leaf_node * leaf, int64_t key, int i);
void write_leaf_val_size(leaf_node * leaf, uint16_t val_size, int i);
void write_leaf_offset(leaf_node * leaf, uint16_t offset, int i);
void write_leaf_value(leaf_node * leaf, const char * val, uint16_t val_size, int i);
void write_temp_body(uint8_t * body, int64_t key, uint16_t val_size, uint16_t offset, const char * val, int i);
void write_leaf_record(leaf_node * leaf, int64_t key, uint16_t val_size, 
                        uint16_t offset, const char * val, int i);
leaf_node * make_leaf( void );
int get_left_index(node * parent, pagenum_t left_pagenum);
void insert_into_leaf( int64_t table_id, pagenum_t leaf_pagenum, int64_t key, const char* value,
                        uint16_t val_size );
void insert_into_leaf_after_splitting( int64_t table_id, pagenum_t leaf_pagenum, int64_t key, const char* value,
                                        uint16_t val_size );
int insert_into_node(int64_t table_id, pagenum_t n_pagenum, int left_index, int64_t key, 
                        pagenum_t right_pagenum);
int insert_into_node_after_splitting(int64_t table_id, pagenum_t old_node_pagenum, int left_index, 
                                        int64_t key, pagenum_t right_pagenum);
int insert_into_parent(int64_t table_id, pagenum_t left_pagenum, int64_t key, pagenum_t right_pagenum);
int insert_into_new_root(int64_t table_id, pagenum_t left, int64_t key, pagenum_t right);
void start_new_tree(int64_t table_id, int64_t key, const char* value,
                        uint16_t val_size);
int insert(int64_t table_id, int64_t key, const char* value,
                uint16_t val_size);

// Deletion.

int get_neighbor_index(int64_t table_id, pagenum_t n_pagenum);
int adjust_root(int64_t table_id, pagenum_t root_pagenum);
void remove_entry_from_node(int64_t table_id, pagenum_t n_pagenum, int key);
int coalesce_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum, 
                    int neighbor_index, int64_t k_prime);
int coalesce_leaf_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum,
                        int neighbor_index, int64_t k_prime);
int redistribute_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum, 
                        int neighbor_index, int k_prime_index, int64_t k_prime);
int redistribute_leaf_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum,
                            int neighbor_index, int k_prime_index, int64_t k_prime);
int delete_entry( int64_t table_id, pagenum_t node_pagenum, int64_t key);
int bpt_delete(int64_t table_id, int64_t key);
#endif /* __BPT_H__*/
