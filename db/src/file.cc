#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "file.h"

// The list of file descriptor
struct table_list_t table_list;

void file_init_table_list(int max_table) {
  table_list.num_of_tables = 0;
  table_list.list.clear();
  table_list.max_num_of_tables = max_table;
}

struct page_t* make_in_momory_page() {
  struct page_t * new_page;
  new_page = (struct page_t*)malloc(PAGE_SIZE);
  if (new_page == NULL) {
    perror("Node creation.");
    exit(EXIT_FAILURE);
  }
  return new_page;
}

// Open existing database file or create one if it doesn't exist
int64_t file_open_table_file(const char* pathname) {
  int64_t table_id;

  if(table_list.num_of_tables >= table_list.max_num_of_tables)
    return -1;

  // Open file
  int fd = open(pathname, O_SYNC | O_CREAT | O_RDWR, 0777);
  uint64_t* check_magic = (uint64_t *)malloc(sizeof(uint64_t));
  int size = pread(fd, check_magic, 8, 0);
  
  // If file already exists
  if(size > 0) {
    // If magic number is correct
    if(*check_magic == 2022) {
      table_id = ++table_list.num_of_tables;
      table_list.list.insert({table_id, fd});
      return table_id;
    }
    else {
      return -1;
    }
  }  
  else {
    table_id = ++table_list.num_of_tables;
    table_list.list.insert({table_id, fd});

    // Create initial header page
    struct header_page_t* header_page = (struct header_page_t *)make_in_momory_page();
    header_page->magic_num = 2022;
    header_page->free_page_num = 0X0001;
    header_page->page_count = INITIAL_DB_NUM_OF_PAGES;
    file_write_page(table_id, 0x0, (struct page_t*)header_page);
    free(header_page);

    // Create pages of initial db
    uint64_t i;
    for(i = 1; i < INITIAL_DB_NUM_OF_PAGES; i++) {
      struct page_t *page = make_in_momory_page();
      if(i + 1 < INITIAL_DB_NUM_OF_PAGES)
        page->next_page = i + 1;
      else
        page->next_page = 0x0;

      file_write_page(table_id, i, page);
      free(page); 
    }
    return table_id;
  }

  return 0;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id) { 
  // Read header page
  struct header_page_t* header_page = (struct header_page_t *)make_in_momory_page();
  file_read_page(table_id, 0x0, (struct page_t*)header_page);
  // If there is no free page left
  if(header_page->free_page_num == 0x0) {
    // Create new pages
    uint64_t i, offset = header_page->page_count;
    for(i = 0; i < offset; i++) {
      struct page_t *page = make_in_momory_page();
      if(i + 1 < offset)
        page->next_page = i + 1 + offset;
      else
        page->next_page = 0x0;

      file_write_page(table_id, i + offset, page);
      free(page);
      }
  
    // Update header page
    header_page->page_count *= 2;
    header_page->free_page_num = 0x0 + offset;
    file_write_page(table_id, 0x0, (struct page_t*)header_page);
  }
  pagenum_t new_alloc_page_num = header_page->free_page_num;
    
  // Allocate page
  struct page_t *new_alloc_page = make_in_momory_page();
  file_read_page(table_id, new_alloc_page_num, new_alloc_page);

  // Update header page
  header_page->free_page_num = new_alloc_page->next_page;
  file_write_page(table_id, 0x0, (struct page_t*)header_page);

  free(new_alloc_page);
  free(header_page);

  return new_alloc_page_num;
}

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t pagenum) {
  // Read header page
  struct header_page_t* header_page = (struct header_page_t *)make_in_momory_page();
  file_read_page(table_id, 0x0, (struct page_t*)header_page);
  
  // Read page wanted to be free
  struct page_t *page = make_in_momory_page();
  file_read_page(table_id, pagenum, page);

  // Insert page to free page list
  page->next_page = header_page->free_page_num;
  header_page->free_page_num = pagenum;
  file_write_page(table_id, 0x0, (struct page_t*)header_page);

  free(page);
  free(header_page);
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int64_t table_id, pagenum_t pagenum, struct page_t* dest) {
  if(table_id > table_list.num_of_tables || table_id < 0)
    return;
  pread(table_list.list[table_id], dest, PAGE_SIZE, PAGE_SIZE * pagenum);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t pagenum, const struct page_t* src) {
  if(table_id > table_list.num_of_tables || table_id < 0)
    return;
  pwrite(table_list.list[table_id], src, PAGE_SIZE, PAGE_SIZE * pagenum);
  sync();

}

// Close the database file
void file_close_table_file() {
  
  // Find table_id in table_id_list and close
  int64_t i;
  for(i = 1; i <= table_list.num_of_tables; i++) {
    close(table_list.list[i]);
  }
  table_list.num_of_tables = 0;
  table_list.list.clear();
}
