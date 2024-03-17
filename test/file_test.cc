#include "file.h"

#include <gtest/gtest.h>

#include <string>

/*******************************************************************************
 * The test structures stated here were written to give you and idea of what a
 * test should contain and look like. Feel free to change the code and add new
 * tests of your own. The more concrete your tests are, the easier it'd be to
 * detect bugs in the future projects.
 ******************************************************************************/

/*
 * Tests file open/close APIs.
 * 1. Open a file and check the descriptor
 * 2. Check if the file's initial size is 10 MiB
 */

TEST(FileInitTest, HandlesInitialization) {
  int64_t table_id;                                 // file descriptor
  std::string pathname = "init_test.db";  // customize it to your test file

  file_init_table_list(20);
  
  // Open a database file
  table_id = file_open_table_file(pathname.c_str());

  // Check if the file is opened
  ASSERT_TRUE(table_id >= 0);

  // Read the header page
  struct header_page_t* header_page = (struct header_page_t *)malloc(PAGE_SIZE);
  file_read_page(table_id, 0x0, (struct page_t*)header_page);

  // Check the size of the initial file
  int num_pages = header_page->page_count;
  EXPECT_EQ(num_pages, INITIAL_DB_FILE_SIZE / PAGE_SIZE)
      << "The initial number of pages does not match the requirement: "
      << num_pages;
  
  free(header_page);

  // Close all database files
  file_close_table_file();

  // Remove the db file
  int is_removed = remove(pathname.c_str());

  ASSERT_EQ(is_removed, 0);
}


// TestFixture for page allocation/deallocation tests

class FileTest : public ::testing::Test {
 protected:
  FileTest() { file_init_table_list(20); pathname = "file_test.db", table_id = file_open_table_file(pathname.c_str()); }

  ~FileTest() {
    if (table_id > 0) {
      file_close_table_file();
      remove(pathname.c_str());
    }
  }

  int64_t table_id;                // table id
  std::string pathname;  // path for the file
};


 // Tests page allocation and free
 // 1. Allocate 2 pages and free one of them, traverse the free page list
 //   and check the existence/absence of the freed/allocated page
 
TEST_F(FileTest, HandlesPageAllocation) {
  pagenum_t allocated_page, freed_page;

  // Allocate the pages
  allocated_page = file_alloc_page(table_id);
  freed_page = file_alloc_page(table_id);

  // Free one page
  file_free_page(table_id, freed_page);

  // Read the header page
  struct header_page_t* header_page = (struct header_page_t *)malloc(PAGE_SIZE);
  file_read_page(table_id, 0x0, (struct page_t*)header_page);
  pagenum_t traverse_page = header_page->free_page_num;

  // Traverse the free page list and check the existence of the freed/allocated pages.
  int alloc_page_check = 0, freed_page_check = 0;
  while(1) {
    if(traverse_page == 0x0)
      break;
    
    if(traverse_page == allocated_page)
      alloc_page_check = 1;
    if(traverse_page == freed_page)
      freed_page_check = 1;

    struct page_t *traverse_page_data = (struct page_t *)malloc(PAGE_SIZE);
    file_read_page(table_id, traverse_page, traverse_page_data);

    traverse_page = traverse_page_data->next_page;
    free(traverse_page_data);
  }
  free(header_page);

  // Check allocated page is not in the free page list and freed page is in the list
  ASSERT_EQ(alloc_page_check, 0);
  ASSERT_EQ(freed_page_check, 1);
}


// Tests page read/write operations
// 1. Write/Read a page with some random content and check if the data matches
TEST_F(FileTest, CheckReadWriteOperation) {
  char *src = (char*)malloc(PAGE_SIZE * sizeof(char)), *dest = (char*)malloc(PAGE_SIZE * sizeof(char));
  // Fill src with 'a' ex)src == "aaaaaa..."
  int i;
  for(i = 0; i < PAGE_SIZE; i++)
    src[i] = 'a';

  // Write src in the allocated page and read it to dest
  pagenum_t pagenum = file_alloc_page(table_id);
  file_write_page(table_id, pagenum, (struct page_t*)src);
  file_read_page(table_id, pagenum, (struct page_t*)dest);
  // Check src is equal to dest
  for(i = 0; i < 1; i++)
    EXPECT_EQ(src[i], dest[i]);

  free(src);
  free(dest);
}
