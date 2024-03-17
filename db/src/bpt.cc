/*
 *  bpt.c  
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 */

#include <queue>
#include "bpt.h"

#define PAGE_BODY_OFFSET 1984

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES.

// Prints all the tree node.
void print_bpt( int64_t table_id ) {
    header_node * header = (header_node *)buffer_read_page(table_id, 0x0);
    pagenum_t root_pagenum = header->root_page_num;
    buffer_page_unlatch((struct page_t *)header);
    int print_value_sign = 0;
    int i, j;
    node * c;

    if (root_pagenum == 0) {
        printf("Empty tree.\n");
        return;
    }

    std::queue<int64_t> q;

    q.push(root_pagenum);
    while(!q.empty()) {
        int64_t cur_pagenum = q.front();
        q.pop();

        printf("<%lu> ", cur_pagenum);

        c = (node *)buffer_read_page(table_id, cur_pagenum);

        if (!c->is_leaf) {
            q.push(c->leftmost_page_num);
            for(i = 0; i < c->num_keys; i++) {
                printf("%lu ", c->entries[i * 2]);
                q.push(c->entries[i * 2 + 1]);
            }
        }
        else {
            for(i = 0; i < ((leaf_node *)c)->num_keys; i++)
                printf("%lu ", read_leaf_key((leaf_node *)c, i));
        }
        printf(" | ");
        buffer_page_unlatch((struct page_t *)c);
    }
    printf("\n");
}


// Prints the bottom row of keys and values of the tree.
void print_leaves( int64_t table_id ) {
    header_node * header = (header_node *)buffer_read_page(table_id, 0x0);
    pagenum_t root_pagenum = header->root_page_num;
    buffer_page_unlatch((struct page_t *)header);
    int print_value_sign = 1;

    if (root_pagenum == 0) {
        printf("Empty tree.\n");
        return;
    }

    int i, j;
    char * print_value = (char *)malloc(112 * sizeof(char));
    node * c = (node *)buffer_read_page(table_id, root_pagenum);
    
    while (!c->is_leaf) {
        buffer_page_unlatch((struct page_t *)c);
        c = (node *)buffer_read_page(table_id, c->leftmost_page_num);
    }
    while (true) {
        for (i = 0; i < c->num_keys; i++) {
            printf("%lu ", read_leaf_key((leaf_node *)c, i));
            if(print_value_sign) {
                read_leaf_value((leaf_node *)c, print_value, i);
                for(j = 0; j < read_leaf_val_size((leaf_node *)c, i); j++)
                    printf("%c", (char)print_value[j]);
                printf(" ");
            }
        }
        if (((leaf_node *)c)->right_sibling_page_num != 0) {
            printf(" | ");
            buffer_page_unlatch((struct page_t *)c);
            c = (node *)buffer_read_page(table_id, ((leaf_node *)c)->right_sibling_page_num);
        }
        else
            break;
    }
    buffer_page_unlatch((struct page_t *)c);
    free(print_value);
    printf("\n");
}


/* Finds keys and their pointers, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_pointers, and returns the number of
 * entries found.
 */
int find_range( int64_t table_id, int64_t begin_key, int64_t end_key, 
                std::vector<int64_t>* keys, std::vector<char*>* values,
                std::vector<uint16_t>* val_sizes) {
    int i, num_found;
    num_found = 0;
    char * temp_values;

    // Find leaf node that has the begin key.
    pagenum_t n_pagenum = find_leaf( table_id, begin_key);
    leaf_node * n = (leaf_node *)buffer_read_page(table_id, n_pagenum);

    // Fill return vectors.
    if (n_pagenum == 0) return 0;
    for (i = 0; i < n->num_keys && read_leaf_key(n, i) < begin_key; i++);
    if (i == n->num_keys) return 0;
    while (n_pagenum != 0) {
        buffer_page_unlatch((struct page_t *)n);
        n = (leaf_node *)buffer_read_page(table_id, n_pagenum);
        for ( ; i < n->num_keys && read_leaf_key(n, i) <= end_key; i++) {
            (*keys).push_back(read_leaf_key(n, i));
            (*val_sizes).push_back(read_leaf_val_size(n, i));
            temp_values = (char *)malloc(120 * sizeof(char));
            read_leaf_value(n, temp_values, i);
            (*values).push_back(temp_values);
            num_found++;
        }
        n_pagenum = n->right_sibling_page_num;
        i = 0;
    }
    buffer_page_unlatch((struct page_t *)n);
    return num_found;
}


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
pagenum_t find_leaf( int64_t table_id, int64_t key ) {
    int i = 0;

    node * c = (node *)buffer_read_page(table_id, 0x0);
    pagenum_t pagenum = ((header_node *)c)->root_page_num;
    buffer_page_unlatch((struct page_t *)c);
    if (pagenum == 0)
        return -1;
    c = (node *)buffer_read_page(table_id, pagenum);

    while (!c->is_leaf) {
        i = 0;
        while (i < c->num_keys) {
            if (key >= c->entries[i * 2]) i++;
            else break;
        }
        if (i == 0) {
            pagenum = c->leftmost_page_num;
            buffer_page_unlatch((struct page_t *)c);
            c = (node *)buffer_read_page(table_id, pagenum);
        }
        else {
            pagenum = c->entries[i * 2 - 1];
            buffer_page_unlatch((struct page_t *)c);
            c = (node *)buffer_read_page(table_id, pagenum);
        }
    }
    buffer_page_unlatch((struct page_t *)c);
    return pagenum;
}


// Finds and returns the record to which a key refers.
int find( int64_t table_id, int64_t key, char* ret_val, 
            uint16_t * val_size ) {
    int i = 0;
    leaf_node * c;

    pagenum_t leaf_pagenum = find_leaf( table_id, key );
    if(leaf_pagenum == -1)
        return -1;
    c = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);

    if (leaf_pagenum == 0) return -1;
    for (i = 0; i < c->num_keys; i++)
        if (read_leaf_key(c, i) == key) break;
    if (i == c->num_keys) {
        buffer_page_unlatch((struct page_t *)c);
        return -1;
    }
    else {
        *val_size = read_leaf_val_size(c, i);
        read_leaf_value(c, ret_val, i);
    }
    buffer_page_unlatch((struct page_t *)c);
    return 0;
}

int64_t read_leaf_key(leaf_node * leaf, int i) {
    int64_t key = 0;
    int j;
    for(j = 0; j < 8; j++) {
        key |= leaf->body[i * 12 + j];
        if(j < 7) key <<= 8;
    }
    return key;
}
uint16_t read_leaf_val_size(leaf_node * leaf, int i) {
    uint64_t val_size = 0;
    val_size |= leaf->body[i * 12 + 8];
    val_size <<= 8;
    val_size |= leaf->body[i * 12 + 9];
    return val_size;
}
uint16_t read_leaf_offset(leaf_node * leaf, int i) {
    uint64_t offset = 0;
    offset |= leaf->body[i * 12 + 10];
    offset <<= 8;
    offset |= leaf->body[i * 12 + 11];
    return offset;
}
void read_leaf_value(leaf_node * leaf, char * ret_val, int i) {
    int j;
    for(j = 0; j < read_leaf_val_size(leaf, i); j++)
        ret_val[j] = (char)leaf->body[read_leaf_offset(leaf, i) + j];
}
uint64_t read_temp_body_key(uint8_t * body, int i) {
    int64_t key = 0;
    int j;
    for(j = 0; j < 8; j++) {
        key |= body[i * 12 + j];
        if(j < 7) key <<= 8;
    }
    return key;
}
uint16_t read_temp_body_val_size(uint8_t * body, int i) {
    uint64_t val_size = 0;
    val_size |= body[i * 12 + 8];
    val_size <<= 8;
    val_size |= body[i * 12 + 9];
    return val_size;
}
uint16_t read_temp_body_offset(uint8_t * body, int i) {
    uint64_t offset = 0;
    offset |= body[i * 12 + 10];
    offset <<= 8;
    offset |= body[i * 12 + 11];
    return offset;
}
void read_temp_body_value(uint8_t * body, char * ret_val, int i) {
    int j;
    for(j = 0; j < read_temp_body_val_size(body, i); j++)
        ret_val[j] = (char)body[read_temp_body_offset(body, i) + j];
}



// INSERTION

void write_leaf_key(leaf_node * leaf, int64_t key, int i) {
    int j;
    for(j = 7; j >= 0; j--) {
        leaf->body[i * 12 + j] = 0;
        leaf->body[i * 12 + j] |= key;
        key >>= 8;
    }
}
void write_leaf_val_size(leaf_node * leaf, uint16_t val_size, int i) {
    int j;
    leaf->body[i * 12 + 9] = 0;
    leaf->body[i * 12 + 9] |= val_size;
    val_size >>= 8;
    leaf->body[i * 12 + 8] = 0;
    leaf->body[i * 12 + 8] |= val_size;
}
void write_leaf_offset(leaf_node * leaf, uint16_t offset, int i) {
    int j;
    leaf->body[i * 12 + 11] = 0;
    leaf->body[i * 12 + 11] |= offset;
    offset >>= 8;
    leaf->body[i * 12 + 10] = 0;
    leaf->body[i * 12 + 10] |= offset;
}
void write_leaf_value(leaf_node * leaf, const char * val, uint16_t val_size, int i) {
    int j;
    for(j = 0; j < val_size; j++)
        leaf->body[read_leaf_offset(leaf, i) + j] = (uint8_t)val[j];
}
void write_temp_body(uint8_t * body, int64_t key, uint16_t val_size, uint16_t offset, const char * val, int i){
    int j;
    for(j = 7; j >= 0; j--) {
        body[i * 12 + j] = 0;
        body[i * 12 + j] |= key;
        key >>= 8;
    }
    for(j = 0; j < val_size; j++)
        body[offset + j] = (int8_t)val[j];
    
    body[i * 12 + 9] = 0;
    body[i * 12 + 9] |= val_size;
    val_size >>= 8;
    body[i * 12 + 8] = 0;
    body[i * 12 + 8] |= val_size;

    body[i * 12 + 11] = 0;
    body[i * 12 + 11] |= offset;
    offset >>= 8;
    body[i * 12 + 10] = 0;
    body[i * 12 + 10] |= offset;
}
void write_leaf_record(leaf_node * leaf, int64_t key, uint16_t val_size, 
                        uint16_t offset, const char * val, int i) {
    write_leaf_key(leaf, key, i);
    write_leaf_val_size(leaf, val_size, i);
    write_leaf_offset(leaf, offset, i);
    write_leaf_value(leaf, val, val_size, i);
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
leaf_node * make_leaf( void ) {
    leaf_node * leaf = (leaf_node *)make_in_momory_page();
    leaf->is_leaf = true;
    leaf->num_keys = 0;
    leaf->free_space_amount = LEAF_SPACE_AMOUNT;
    leaf->right_sibling_page_num = 0;
    return leaf;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(node * parent, pagenum_t left_pagenum) {

    int left_index = 0;
    if(parent->leftmost_page_num == left_pagenum)
        return -1;
    while (left_index < parent->num_keys && 
            parent->entries[left_index * 2 + 1] != left_pagenum)
        left_index++;
    return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
void insert_into_leaf( int64_t table_id, pagenum_t leaf_pagenum, int64_t key, const char* value,
                        uint16_t val_size ) {

    int i, insertion_point;
    leaf_node * leaf = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);
    uint16_t temp_offset, offset;

    // Find an offset (minimum offset in the tree - val_Size).
    offset = LEAF_SPACE_AMOUNT;
    for(i = 0; i < leaf->num_keys; i++) {
        temp_offset = read_leaf_offset(leaf, i);
        if(offset > temp_offset)
            offset = temp_offset;
    }
    offset -= val_size;

    // Find an insertion point.
    insertion_point = 0;
    while (insertion_point < leaf->num_keys && read_leaf_key(leaf, insertion_point) < key)
        insertion_point++;

    // Move records that is in right side of the input key.
    for (i = leaf->num_keys; i > insertion_point; i--) {
        write_leaf_key(leaf, read_leaf_key(leaf, i - 1), i);
        write_leaf_val_size(leaf, read_leaf_val_size(leaf, i - 1), i);
        write_leaf_offset(leaf, read_leaf_offset(leaf, i - 1), i);
    }

    // Insert the input record
    write_leaf_record(leaf, key, val_size, offset, value, insertion_point);

    // Update and write the leaf page.
    leaf->num_keys++;
    leaf->free_space_amount -= (12 + val_size);
    buffer_write_page((struct page_t *)leaf);
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting( int64_t table_id, pagenum_t leaf_pagenum, int64_t key, const char* value,
                                        uint16_t val_size ) {                                     
    leaf_node * leaf, * new_leaf;
    pagenum_t new_leaf_pagenum;
    int64_t temp_key;
    uint16_t temp_offset, temp_val_size;
    char temp_value[120] = {};
    int insertion_index, split_index, new_key, temp_size, extra_space = 200, total_num_keys, i, j;
    uint8_t *temp_body = (uint8_t *)malloc(LEAF_SPACE_AMOUNT + extra_space);

    leaf = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);

    // Allocate a new leaf page.
    new_leaf = (leaf_node *)buffer_alloc_page(table_id, &new_leaf_pagenum);
    new_leaf->is_leaf = 1;

    // Find insertion index
    insertion_index = 0;
    while (insertion_index < leaf->num_keys && read_leaf_key(leaf, insertion_index) < key)
        insertion_index++;

    // Copy the leaf node with the new key and value to a temp body.
    total_num_keys = leaf->num_keys + 1;
    temp_size = 0;
    temp_offset = LEAF_SPACE_AMOUNT + extra_space;
    for (i = 0, j = 0; i < total_num_keys; i++, j++) {
        if (j == insertion_index) {
            temp_offset -= val_size;
            write_temp_body(temp_body, key, val_size, temp_offset, value, j);

            temp_size += val_size + 12;
            if(temp_size <= PAGE_BODY_OFFSET)
                split_index = j + 1;
            j++;
        }
        temp_key = read_leaf_key(leaf, i);
        temp_val_size = read_leaf_val_size(leaf, i);
        temp_offset -= temp_val_size;
        read_leaf_value(leaf, temp_value, i);
        write_temp_body(temp_body, temp_key, temp_val_size, temp_offset, temp_value, j);

        temp_size += temp_val_size + 12;
        if(temp_size <= PAGE_BODY_OFFSET)
                split_index = j + 1;
    }
    
    // Copy half of the temp body to original leaf node.
    leaf->num_keys = 0;
    temp_offset = LEAF_SPACE_AMOUNT;
    for (i = 0; i <= split_index; i++) {
        temp_key = read_temp_body_key(temp_body, i);
        temp_val_size = read_temp_body_val_size(temp_body, i);
        temp_offset -= temp_val_size;
        read_temp_body_value(temp_body, temp_value, i);
        
        write_leaf_record(leaf, temp_key, temp_val_size, temp_offset, temp_value, i);
        leaf->num_keys++;
    }
    leaf->free_space_amount = temp_offset - 12 * i;

    // Copy the other half of the temp body to a new leaf node.
    new_leaf->num_keys = 0;
    temp_offset = LEAF_SPACE_AMOUNT;
    for (i = split_index + 1, j = 0; i < total_num_keys; i++, j++) {
        temp_key = read_temp_body_key(temp_body, i);
        temp_val_size = read_temp_body_val_size(temp_body, i);
        temp_offset -= temp_val_size;
        read_temp_body_value(temp_body, temp_value, i);
        write_leaf_record(new_leaf, temp_key, temp_val_size, temp_offset, temp_value, j);
        new_leaf->num_keys++;
    }
    new_leaf->free_space_amount = temp_offset - 12 * i;

    // Update and write two pages.
    new_leaf->right_sibling_page_num = leaf->right_sibling_page_num;
    leaf->right_sibling_page_num = new_leaf_pagenum;

    new_leaf->parent_page_num = leaf->parent_page_num;
    new_key = read_leaf_key(new_leaf, 0);

    buffer_write_page((struct page_t *)leaf);
    buffer_write_page((struct page_t *)new_leaf);

    // Insert the new middle key to the parent.
    insert_into_parent(table_id, leaf_pagenum, new_key, new_leaf_pagenum);
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
int insert_into_node(int64_t table_id, pagenum_t n_pagenum, int left_index, int64_t key, 
                        pagenum_t right_pagenum) {
    int i;
    node * n;

    // Move the entries that is in right side of left_index.
    n = (node *)buffer_read_page(table_id, n_pagenum);
    for (i = n->num_keys - 1; i > left_index; i--) {
        n->entries[(i + 1) * 2]  = n->entries[i * 2];
        n->entries[(i + 1) * 2 + 1] = n->entries[i * 2 + 1];
    }

    // Insert the key and page number.
    n->entries[(left_index + 1) * 2 + 1] = right_pagenum;
    n->entries[(left_index + 1) * 2] = key;

    // Update and write the node.
    n->num_keys++;
    buffer_write_page((struct page_t *)n);
    return 0;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
int insert_into_node_after_splitting(int64_t table_id, pagenum_t old_node_pagenum, int left_index, 
                                        int64_t key, pagenum_t right_pagenum) {
    int i, j, split;
    int64_t  k_prime;
    node * new_node, * old_node, * child;
    uint64_t temp_entries[500], temp_leftmost_pagenum;
    pagenum_t new_node_pagenum;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    old_node = (node *)buffer_read_page(table_id, old_node_pagenum);

    // Copy entire of page entries to a temporary entries
    temp_leftmost_pagenum = old_node->leftmost_page_num;
    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index + 1) j++;
        temp_entries[j * 2] = old_node->entries[i * 2];
        temp_entries[j * 2 + 1] = old_node->entries[i * 2 + 1];
    }

    // Insert the new key and page number
    temp_entries[(left_index + 1) * 2] = key;
    temp_entries[(left_index + 1) * 2 + 1] = right_pagenum;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */ 

    // Split point.
    split = INTERNAL_ORDER / 2;

    // Create the new node.
    new_node = (node *)buffer_alloc_page(table_id, &new_node_pagenum);
    new_node->is_leaf = 0;
    new_node->num_keys = 0;
    new_node->parent_page_num = old_node->parent_page_num;

    // Copy the first half of temp entries to the old node.
    old_node->num_keys = 0;
    old_node->leftmost_page_num = temp_leftmost_pagenum;
    for (i = 0; i < split; i++) {
        old_node->entries[i * 2 + 1] = temp_entries[i * 2 + 1];
        old_node->entries[i * 2] = temp_entries[i * 2];
        old_node->num_keys++;
    }

    // Update k prime (a key that will be inserted to the parent).
    k_prime = temp_entries[i * 2];

    // Copy the other half of temp entries to the new node.
    new_node->leftmost_page_num = temp_entries[i * 2 + 1];
    for (++i, j = 0; i < INTERNAL_ORDER + 1; i++, j++) {
        new_node->entries[j * 2 + 1] = temp_entries[i * 2 + 1];
        new_node->entries[j * 2] = temp_entries[i * 2];
        new_node->num_keys++;
    }

    // Update the parent page number of child nodes.
    child = (node *)buffer_read_page(table_id, new_node->leftmost_page_num);
    child->parent_page_num = new_node_pagenum;
    buffer_write_page((struct page_t *)child);
    for (i = 0; i < new_node->num_keys; i++) {
        child = (node *)buffer_read_page(table_id, new_node->entries[i * 2 + 1]);
        child->parent_page_num = new_node_pagenum;
        buffer_write_page((struct page_t *)child);
    }

    // Write two pages.
    buffer_write_page((struct page_t *)old_node);
    buffer_write_page((struct page_t *)new_node);

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */
    return insert_into_parent(table_id, old_node_pagenum, k_prime, new_node_pagenum);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
int insert_into_parent(int64_t table_id, pagenum_t left_pagenum, int64_t key, pagenum_t right_pagenum) {
    int left_index, parent_num_keys;
    node *parent, *left;
    pagenum_t parent_pagenum;


    // Find the page number of parent node.
    left = (node *)buffer_read_page(table_id, left_pagenum);
    parent_pagenum = left->parent_page_num;
    buffer_page_unlatch((struct page_t *)left);
    
    // Case: new root.
    if (parent_pagenum == 0x0)
        return insert_into_new_root(table_id, left_pagenum, key, right_pagenum);

    // Case: leaf or node.
    
    parent = (node *)buffer_read_page(table_id, parent_pagenum);
    left_index = get_left_index(parent, left_pagenum);
    parent_num_keys = parent->num_keys;
    buffer_page_unlatch((struct page_t *)parent);

    // Simple case: the new key fits into the node.
    if (parent_num_keys < INTERNAL_ORDER)
        return insert_into_node(table_id, parent_pagenum, left_index, key, right_pagenum);

    // Harder case:  split a node
    return insert_into_node_after_splitting(table_id, parent_pagenum, left_index, key, right_pagenum);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
int insert_into_new_root(int64_t table_id, pagenum_t left_pagenum, int64_t key, pagenum_t right_pagenum) {
    pagenum_t root_pagenum;
    header_node * header;
    node * root, * left, * right;
    header = (header_node *)buffer_read_page(table_id, 0x0);
    buffer_page_unlatch((struct page_t *)header);

    // Allocate a new root and initialize it.
    root = (node *)buffer_alloc_page(table_id, &root_pagenum);
    root->is_leaf = 0;
    root->entries[0] = key;
    root->leftmost_page_num = left_pagenum;
    root->entries[1] = right_pagenum;
    root->num_keys = 1;
    root->parent_page_num = 0x0;
    buffer_write_page((struct page_t*)root);

    // Upadate the header page.
    header = (header_node *)buffer_read_page(table_id, 0x0);
    header->root_page_num = root_pagenum;
    buffer_write_page((struct page_t *)header);

    // Update the parent page number of left and right nodes.
    left = (node *)buffer_read_page(table_id, left_pagenum);
    right = (node *)buffer_read_page(table_id, right_pagenum);
    left->parent_page_num = root_pagenum;
    right->parent_page_num = root_pagenum;
    buffer_write_page((struct page_t *)left);
    buffer_write_page((struct page_t *)right);

    return 0;
}



// First insertion: start a new tree.
void start_new_tree(int64_t table_id, int64_t key, const char* value,
                uint16_t val_size) {
    header_node * header;
    leaf_node * root;
    pagenum_t root_num;

    // Allocate a root page.
    root = (leaf_node *)buffer_alloc_page(table_id, &root_num);

    // Update the header page.
    header = (header_node *)buffer_read_page(table_id, 0x0);
    header->root_page_num = root_num;
    buffer_write_page((struct page_t *)header);

    // Initialize the root page.
    root->parent_page_num = 0x0;
    root->is_leaf = 1;
    root->num_keys = 1;
    root->right_sibling_page_num = 0x0;
    root->free_space_amount = LEAF_SPACE_AMOUNT - (val_size + 12);

    // Insert the key and value.
    write_leaf_key(root, key, 0);
    write_leaf_val_size(root, val_size, 0);
    write_leaf_offset(root, LEAF_SPACE_AMOUNT - val_size, 0);
    write_leaf_value(root, value, val_size, 0);

    buffer_write_page((struct page_t *)root);
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int insert(int64_t table_id, int64_t key, const char* value,
                uint16_t val_size) {
    
    leaf_node * leaf;
    pagenum_t leaf_pagenum; 

    node * c = (node *)buffer_read_page(table_id, 0x0);
    pagenum_t root = ((header_node *)c)->root_page_num;
    buffer_page_unlatch((struct page_t *)c);

    // Case: the tree does not exist yet, start a new tree.
    if (root == 0x0) {
        start_new_tree(table_id, key, value, val_size);
        return 0;
    }

    // The current implementation ignores duplicates.
    char temp_value[120] = {};
    uint16_t temp_val_size;
    if (find(table_id, key, temp_value, &temp_val_size) >= 0)
        return -1;

    // Case: the tree already exists.

    // Find the leaf node the input record should go in.
    leaf_pagenum = find_leaf(table_id, key);
    leaf = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);
    uint64_t leaf_free_space_amout = leaf->free_space_amount;
    buffer_page_unlatch((struct page_t *)leaf);

    // Case: leaf has room for key and pointer.
    if (leaf_free_space_amout >= val_size + 12) {
        insert_into_leaf(table_id, leaf_pagenum, key, value, val_size);
        return 0;
    }

    // Case:  leaf must be split.
    insert_into_leaf_after_splitting(table_id, leaf_pagenum, key, value, val_size);
    return 0;
}


// Deletion

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(int64_t table_id, pagenum_t n_pagenum, pagenum_t parent_pagenum) {

    int i;
    node * parent;

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    parent = (node *)buffer_read_page(table_id, parent_pagenum);
    if(parent->leftmost_page_num == n_pagenum) {
        buffer_page_unlatch((struct page_t *)parent);
        return -2;
    }
    for (i = 0; i < parent->num_keys; i++) {
        if (parent->entries[i * 2 + 1] == n_pagenum) {
            buffer_page_unlatch((struct page_t *)parent);
            return i - 1;
        }
    }

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    exit(EXIT_FAILURE);
}
int adjust_root(int64_t table_id, pagenum_t root_pagenum) {

    node * root, * new_root;
    header_node * header;

    root = (node *)buffer_read_page(table_id, root_pagenum);

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0) {
        buffer_page_unlatch((struct page_t *)root);
        return 0;
    }

    /* Case: empty root. 
     */

    header = (header_node *)buffer_read_page(table_id, 0x0);
    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        header->root_page_num = root->leftmost_page_num;
        new_root = (node *)buffer_read_page(table_id, root->leftmost_page_num);
        new_root->parent_page_num = 0x0;
        buffer_write_page((struct page_t *)new_root);
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else {
        header->root_page_num = 0x0;
    }

    buffer_write_page((struct page_t *)header);
    buffer_free_page((struct page_t *)root);
    return 0;
}

void remove_entry_from_node(int64_t table_id, pagenum_t n_pagenum, int key) {
    int i;
    node * n;

    n = (node *)buffer_read_page(table_id, n_pagenum);
    // Case: n is an internal node;
    if (!n->is_leaf) {
        i = 0;
        // Remove the key and shift other keys accordingly.
        while (n->entries[i * 2] != key)
            i++;
        for(++i; i < n->num_keys; i++) {
            n->entries[(i - 1) * 2] = n->entries[i * 2];
            n->entries[(i - 1) * 2 + 1] = n->entries[i * 2 + 1];
        }
        n->num_keys--;
        buffer_write_page((struct page_t *)n);
        return;
    }

    // Case: n is a leaf node;
    int j, min_offset = PAGE_SIZE;
    leaf_node * leaf_n;
    uint16_t offset_diff, del_record_offset, temp_offset;

    // Find the location of the key
    leaf_n = (leaf_node *)n;
    i = 0;
    while (read_leaf_key(leaf_n, i) != key)
        i++;

    del_record_offset = read_leaf_offset(leaf_n, i);
    offset_diff = read_leaf_val_size(leaf_n, i);

    // Update offsets of records whose offset is smaller than deleted record
    for(j = 0; j < leaf_n->num_keys; j++) {
        temp_offset = read_leaf_offset(leaf_n, j);
        if(min_offset > temp_offset)
            min_offset = temp_offset;
        if(temp_offset < del_record_offset)
            write_leaf_offset(leaf_n, temp_offset + offset_diff, j);
    }

    // Move values of offset-updated records
    for(j = del_record_offset - 1; j >= min_offset; j--)
        leaf_n->body[j + offset_diff] = leaf_n->body[j];

    // Shift keys, val_sizes and offsets.
    for (++i; i < leaf_n->num_keys; i++) {
        write_leaf_key(leaf_n, read_leaf_key(leaf_n, i), i - 1);
        write_leaf_val_size(leaf_n, read_leaf_val_size(leaf_n, i), i - 1);
        write_leaf_offset(leaf_n, read_leaf_offset(leaf_n, i), i - 1);
    }
    leaf_n->num_keys--;
    leaf_n->free_space_amount += 12 + offset_diff;
    buffer_write_page((struct page_t *)leaf_n);
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
/* Case:  nonleaf node.
 * Append k_prime and the following pointer.
 * Append all pointers and keys from the neighbor.
 */
int coalesce_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum, 
                    int neighbor_index, int64_t k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp, * n, * neighbor;
    pagenum_t parent_pagenum;
    n = (node *)buffer_read_page(table_id, n_pagenum);
    neighbor = (node *)buffer_read_page(table_id, neighbor_pagenum);

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -2) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
        neighbor_pagenum = n_pagenum;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    // Append k_prime.
    neighbor->entries[neighbor_insertion_index * 2] = k_prime;
    neighbor->num_keys++;

    // Copy entries from n to neighbor.
    n_end = n->num_keys;
    neighbor->entries[neighbor_insertion_index * 2 + 1] = n->leftmost_page_num;
    for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
        neighbor->entries[i * 2] = n->entries[j * 2];
        neighbor->entries[i * 2 + 1] = n->entries[j * 2 + 1];
        neighbor->num_keys++;
        n->num_keys--;
    }

    //  All children must now point up to the same parent.
    tmp = (node *)buffer_read_page(table_id, neighbor->leftmost_page_num);
    tmp->parent_page_num = neighbor_pagenum;
    buffer_write_page((struct page_t *)tmp);
    for (i = 0; i < neighbor->num_keys; i++) {
        tmp = (node *)buffer_read_page(table_id, neighbor->entries[i * 2 + 1]);
        tmp->parent_page_num = neighbor_pagenum;
        buffer_write_page((struct page_t *)tmp);
    }

    parent_pagenum = n->parent_page_num;

    buffer_free_page((struct page_t *)n);
    buffer_page_unlatch((struct page_t *)neighbor);
    delete_entry(table_id, parent_pagenum, k_prime);
    return 0;
}
/* In a leaf, append the keys and pointers of
 * n to the neighbor.
 * Set the neighbor's last pointer to point to
 * what had been n's right neighbor.
 */
int coalesce_leaf_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum,
                            int neighbor_index, int64_t k_prime) {
         
    int i, j, neighbor_insertion_index;
    leaf_node * tmp, * n, * neighbor;
    int16_t temp_val_size, current_offset, temp_offset;
    char temp_value[120] = {};
    
    n = (leaf_node *)buffer_read_page(table_id, n_pagenum);
    neighbor = (leaf_node *)buffer_read_page(table_id, neighbor_pagenum);

    // Swap neighbor with node if node is on the
    // extreme left and neighbor is to its right.
    if (neighbor_index == -2) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    // Starting point in the neighbor for copying records from n.
    neighbor_insertion_index = neighbor->num_keys;

    // Find an current offset of neighbor (minimum offset between records).
    current_offset = LEAF_SPACE_AMOUNT;
    for(i = 0; i < neighbor->num_keys; i++) {
        temp_offset = read_leaf_offset(neighbor, i);
        if(current_offset > temp_offset)
            current_offset = temp_offset;
    }

    // Copy records from n to neighbor.
    for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
        temp_val_size = read_leaf_val_size(n, j);
        current_offset -= temp_val_size;
        read_leaf_value(n, temp_value, j);
        write_leaf_key(neighbor, read_leaf_key(n, j), i);
        write_leaf_val_size(neighbor, temp_val_size, i);
        write_leaf_offset(neighbor, current_offset, i);
        write_leaf_value(neighbor, temp_value, temp_val_size, i);
        neighbor->num_keys++;
        neighbor->free_space_amount -= 12 + temp_val_size;
    }
    neighbor->right_sibling_page_num = n->right_sibling_page_num;

    buffer_free_page((struct page_t *)n);
    buffer_page_unlatch((struct page_t *)neighbor);
    delete_entry(table_id, n->parent_page_num, k_prime);

    return 0;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
int redistribute_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum,
                        int neighbor_index, int k_prime_index, int64_t k_prime) {  

    int i;
    node * tmp, * parent, * n, * neighbor;
    n = (node *)buffer_read_page(table_id, n_pagenum);
    neighbor = (node *)buffer_read_page(table_id, neighbor_pagenum);

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -2) {
        for (i = n->num_keys; i > 0; i--) {
            n->entries[i * 2] = n->entries[(i - 1) * 2];
            n->entries[i * 2 + 1] = n->entries[(i - 1) * 2 + 1];
        }
        n->entries[0 * 2 + 1] = n->leftmost_page_num;

        n->entries[0 * 2] = k_prime;

        n->leftmost_page_num = neighbor->entries[(neighbor->num_keys - 1) * 2 + 1];

        tmp = (node *)buffer_read_page(table_id, n->leftmost_page_num);
        tmp->parent_page_num = n_pagenum;
        buffer_write_page((struct page_t *)tmp);   

        parent = (node *)buffer_read_page(table_id, n->parent_page_num);
        parent->entries[k_prime_index * 2] = neighbor->entries[(neighbor->num_keys - 1) * 2];
        buffer_write_page((struct page_t *)parent);
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        n->entries[n->num_keys * 2] = k_prime;
        n->entries[n->num_keys * 2 + 1] = neighbor->leftmost_page_num;

        tmp = (node *)buffer_read_page(table_id, n->entries[n->num_keys * 2 + 1]);
        tmp->parent_page_num = n_pagenum;
        buffer_write_page((struct page_t *)tmp);

        parent = (node *)buffer_read_page(table_id, n->parent_page_num);
        parent->entries[k_prime_index * 2] = neighbor->entries[0 * 2];
        buffer_write_page((struct page_t *)parent);

        neighbor->leftmost_page_num = neighbor->entries[0 * 2 + 1];
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->entries[i * 2] = neighbor->entries[(i + 1) * 2];
            neighbor->entries[i * 2 + 1] = neighbor->entries[(i + 1) * 2 + 1];
        }
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;
    buffer_write_page((struct page_t *)n);
    buffer_write_page((struct page_t *)neighbor);
    return 0;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
int redistribute_leaf_nodes(int64_t table_id, pagenum_t n_pagenum, pagenum_t neighbor_pagenum,
                            int neighbor_index, int k_prime_index, int64_t k_prime) {  
                 
    node * parent;
    leaf_node * n, * neighbor;
    int64_t temp_key;
    int16_t temp_val_size;
    char temp_value[120] = {};

    n = (leaf_node *)buffer_read_page(table_id, n_pagenum);
    neighbor = (leaf_node *)buffer_read_page(table_id, neighbor_pagenum);
    parent = (node *)buffer_read_page(table_id, n->parent_page_num);

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -2) {
        temp_key = read_leaf_key(neighbor, neighbor->num_keys - 1);
        temp_val_size = read_leaf_val_size(neighbor, neighbor->num_keys - 1);
        read_leaf_value(neighbor, temp_value, neighbor->num_keys - 1);

        insert_into_leaf(table_id, n_pagenum, temp_key, temp_value, temp_val_size);
        remove_entry_from_node(table_id, neighbor_pagenum, temp_key);

        parent->entries[k_prime_index * 2] = read_leaf_key(n, 0);
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        temp_key = read_leaf_key(neighbor, 0);
        temp_val_size = read_leaf_val_size(neighbor, 0);
        read_leaf_value(neighbor, temp_value, 0);

        insert_into_leaf(table_id, n_pagenum, temp_key, temp_value, temp_val_size);
        remove_entry_from_node(table_id, neighbor_pagenum, temp_key);

        parent->entries[k_prime_index * 2] = read_leaf_key(neighbor, 0);
    }

    buffer_page_unlatch((struct page_t *)parent);
    buffer_write_page((struct page_t *)n);
    buffer_write_page((struct page_t *)neighbor);

    return 0;
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
int delete_entry( int64_t table_id, pagenum_t node_pagenum, int64_t key) {

    node * neighbor, * n, * parent;
    int neighbor_index;
    pagenum_t neighbor_pagenum;
    int k_prime_index;
    int64_t k_prime;
    int capacity, threshold;

    // Remove key and pointer from node.
    remove_entry_from_node(table_id, node_pagenum, key);

    // Case: deletion from the root.
    header_node * header = (header_node *)buffer_read_page(table_id, 0x0);
    pagenum_t root_pagenum = header->root_page_num;
    buffer_page_unlatch((struct page_t *)header);
    if (node_pagenum == root_pagenum) {
        return adjust_root(table_id, node_pagenum);
    }


    /* Case: deletion from a node below the root.
     * (Rest of function body.)
     */

    n = (node *)buffer_read_page(table_id, node_pagenum);
    
    // Determine threshold.
    threshold = n->is_leaf ? 2500 : 124;

    // Case: node stays at or above minimum.
    if(!n->is_leaf && n->num_keys > threshold
        || n->is_leaf && ((leaf_node *)n)->free_space_amount < threshold) {
        buffer_page_unlatch((struct page_t *)n);
        return 0;
    }

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    // Find index of neighbor and k_prime.
    neighbor_index = get_neighbor_index(table_id, node_pagenum, n->parent_page_num);
    parent = (node *)buffer_read_page(table_id, n->parent_page_num);
    k_prime_index = neighbor_index == -2 ? 0 : neighbor_index + 1;
    k_prime = parent->entries[k_prime_index * 2];

    // Find neighbor page number.
    if (neighbor_index == -2)
        neighbor_pagenum = parent->entries[0 * 2 + 1];
    else if (neighbor_index == -1)
        neighbor_pagenum = parent->leftmost_page_num;
    else
        neighbor_pagenum = parent->entries[neighbor_index * 2 + 1];

    buffer_page_unlatch((struct page_t *)parent);

    neighbor = (node *)buffer_read_page(table_id, neighbor_pagenum);

    capacity = n->is_leaf ? LEAF_SPACE_AMOUNT : INTERNAL_ORDER;
    

    if(!n->is_leaf) {
        // Internal node coalescence.
        if (neighbor->num_keys + n->num_keys < capacity) {
            buffer_page_unlatch((struct page_t *)n);
            buffer_page_unlatch((struct page_t *)neighbor);
            return coalesce_nodes(table_id, node_pagenum, neighbor_pagenum, neighbor_index, k_prime);
        }
        // Internal node redistribution.
        else {
            buffer_page_unlatch((struct page_t *)n);
            buffer_page_unlatch((struct page_t *)neighbor);
            return redistribute_nodes(table_id, node_pagenum, neighbor_pagenum, neighbor_index, k_prime_index, k_prime);
        }
    }
    else {
        // Leaf node coalescence.
        if (((leaf_node *)neighbor)->free_space_amount + ((leaf_node *)n)->free_space_amount > capacity) {
            buffer_page_unlatch((struct page_t *)n);
            buffer_page_unlatch((struct page_t *)neighbor);
            return coalesce_leaf_nodes(table_id, node_pagenum, neighbor_pagenum, neighbor_index, k_prime);
        }

        // Leaf node redistribution.
        else {
            while (((leaf_node *)n)->free_space_amount >= threshold)
                redistribute_leaf_nodes(table_id, node_pagenum, neighbor_pagenum,
                                            neighbor_index, k_prime_index, k_prime);
            buffer_page_unlatch((struct page_t *)n);
            buffer_page_unlatch((struct page_t *)neighbor);
            return 0;
        }
    }
    
    buffer_page_unlatch((struct page_t *)n);
    buffer_page_unlatch((struct page_t *)neighbor);
    return 0;
}



/* Master deletion function.
 */
int bpt_delete(int64_t table_id, int64_t key) {
    pagenum_t key_leaf_pagenum;

    // Find the leaf node that has the key.
    key_leaf_pagenum = find_leaf(table_id, key);

    // Case: the key doesn't exist.
    if (key_leaf_pagenum < 0)
        return -1;

    // Case: the key exists.
    return delete_entry(table_id, key_leaf_pagenum, key);
}