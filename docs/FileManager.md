# File Manager

The File Manager manages disk space for upper layers such as the buffer management layer or index management layer. It distinguishes between 'free spaces' and 'allocated spaces' to inform where the location can be used. Additionally, it converts logical page numbers into physical addresses so that upper layers do not have to know about the actual physical locations.

There are six functions in this file manager:

1. **file_open_database_file**: This function opens the database file and checks the magic number. If the magic number is different, it returns -1. If there isn't the same name of the database file, it creates a new file and paginates 10MB. After these processes, it returns the file descriptor.

2. **file_alloc_page**: This function allocates a page and returns the allocated page number. If the free page list is empty, it doubles the size of the paginated file and allocates one page.

3. **file_free_page**: This function frees an allocated page.

4. **file_read_page**: This function reads the page from the database file.

5. **file_write_page**: This function writes the page to the database file.

6. **file_close_database_file**: This function closes all the database files that are opened.

## Unittests with GoogleTest

It tests the database system to work well using GoogleTest.

There are three unit tests for the file manager:

1. **File Initialization**: This function tests "file open", "file size", and "file close". It calls `file_open_database_file` to test whether the file is opened correctly and checks the number of pages. Then, it calls `file_close_database_file` to check that the file is closed properly.

2. **Page Management**: This function tests "page allocate/free". It calls `file_alloc_page` twice to allocate two pages and calls `file_free_page` once to free one of those pages. Then, it checks whether the freed page is in the free page list and whether the allocated page is not in the list.

3. **Page I/O**: This function tests "page read/write". It allocates a page and fills it with specific data. Then, it calls `file_write_page` to write the page in the file and calls `file_read_page` to read that file for comparing it with the original one. If they are the same, the test will pass.
