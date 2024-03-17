#include "buffer.h"

#include <gtest/gtest.h>

#include <string>
#include <stdlib.h>

#define BUFFER_SIZE 10

class BufferTest : public ::testing::Test {
 protected:
  /*
   * NOTE: You can also use constructor/destructor instead of SetUp() and
   * TearDown(). The official document says that the former is actually
   * perferred due to some reasons. Checkout the document for the difference
   */
  BufferTest() { 
    file_init_table_list(20); buffer_init(BUFFER_SIZE);  
    pathname = "Buffer_test.db", table_id = file_open_table_file(pathname.c_str()); 
    }

  ~BufferTest() {
    if (table_id > 0) {
      buffer_clear();
      file_close_table_file();
      remove(pathname.c_str());
    }
  }

  int64_t table_id;      // table id
  std::string pathname;  // path for the file
};

TEST_F(BufferTest, CheckBufferReadWriteOperation) {
  pagenum_t pagenum;

  char * src = (char *)buffer_alloc_page(table_id, &pagenum);

  // Fill src with 'a' ex)src == "aaaaaa..."
  int i;
  for(i = 0; i < PAGE_SIZE; i++)
    src[i] = 'a';

  // Write src in the allocated page and read it to dest
  buffer_write_page((struct page_t *)src);
  char * dest = (char *)buffer_read_page(table_id, pagenum);

  // Check src is equal to dest
  for(i = 0; i < PAGE_SIZE; i++)
    EXPECT_EQ(src[i], dest[i]);
}

TEST_F(BufferTest, CheckBufferReplacement) {
  pagenum_t temp_pagenum, pagenum;
  struct page_t * temp;

  // Allocate pages on buffer without unpin.
  int i;
  for(i = 0; i < BUFFER_SIZE; i++)
    temp = buffer_alloc_page(table_id, &temp_pagenum);
  
  // Write one page.
  temp->next_page = 2019092306;
  pagenum = temp_pagenum;
  buffer_write_page(temp);

  // Allocate a page once again to test buffer writing on the disk.
  temp = buffer_alloc_page(table_id, &temp_pagenum);
  buffer_page_unlatch(temp);

  // Read written page.
  temp = buffer_read_page(table_id, pagenum);

  // Check the page isn't change.
  EXPECT_EQ(temp->next_page, 2019092306);
}
