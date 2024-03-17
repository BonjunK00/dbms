#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "file.h"

struct buffer_t {
    int8_t frame[PAGE_SIZE];
    int64_t table_id;
    pagenum_t pagenum;
    int is_dirty;
    pthread_mutex_t page_latch;
    struct buffer_t * next;
    struct buffer_t * prev;
};

struct buffer_pool {
    struct buffer_t * list;
    int num_buf;
    struct buffer_t * LRU_begin, * LRU_end;
};

// Check the page and return if it exists.
int buffer_check(int64_t  table_id, pagenum_t pagenum);

// Unpin the page.
void buffer_page_unlatch(struct page_t * page);

// Allocate a new page and return the page.
struct page_t * buffer_alloc_page(int64_t table_id, pagenum_t * ret_pagenum);

// Free the page and add the page to LRU list
void buffer_free_page(struct page_t * page);

// Read an on-disk page into the in-memory page structure(dest)
struct page_t * buffer_read_page(int64_t table_id, pagenum_t pagenum);

// Write an in-memory page(src) to the on-disk page
void buffer_write_page(struct page_t * dirty_page);

// Initalizing.
void buffer_init(int num_buf);

// Clear.
void buffer_clear();

#endif  // BUFFER_H_
