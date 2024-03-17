#ifndef __FILE_H__
#define __FILE_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include "page.h"

#define INITIAL_DB_FILE_SIZE (10 * 1024 * 1024)  // 10 MiB
#define PAGE_SIZE (4 * 1024)                     // 4 KiB
#define INITIAL_DB_NUM_OF_PAGES (INITIAL_DB_FILE_SIZE / PAGE_SIZE) // 2560

struct table_list_t {
    int64_t num_of_tables;
    std::map< int64_t, int > list;
    int max_num_of_tables;
};

void file_init_table_list(int max_table);

struct page_t* make_in_momory_page();

// Open existing database file or create one if it doesn't exist
int64_t file_open_table_file(const char* pathname);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id);

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src);

// Close the database file
void file_close_table_file();

#endif  // DB_FILE_H_
