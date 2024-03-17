#ifndef __PAGE_H__
#define __PAGE_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define LEAF_SPACE_AMOUNT 3968
#define INTERNAL_ORDER 248

typedef uint64_t pagenum_t;

struct page_t {
    pagenum_t next_page;

    int8_t reserved[4088];
};

struct header_page_t {
    uint64_t magic_num;
    pagenum_t free_page_num;
    uint64_t page_count;
    pagenum_t root_page_num;

    int8_t reserved[4064]; 
};

struct leaf_page_t {
    pagenum_t parent_page_num;
    uint32_t is_leaf;
    uint32_t num_keys;

    int8_t reserved[96];

    uint64_t free_space_amount;
    pagenum_t right_sibling_page_num;

    uint8_t body[3968];
};

struct internal_page_t {
    pagenum_t parent_page_num;
    uint32_t is_leaf;
    uint32_t num_keys;

    int8_t reserved[104];

    pagenum_t leftmost_page_num;

    int64_t entries[496];
};

#endif