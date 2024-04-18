# Buffer Manager

The Buffer Manager is designed to enhance the speed of reading and writing pages. It utilizes "buffers" to store pages in memory. When a user requests to read a page, the Buffer Manager quickly returns the page from memory if it is already present. If the page is not in memory, it reads the page from disk and then returns it.

## Design

A buffer structure consists of a "frame" and buffer information. The frame represents the space for loading on-disk pages. Buffer information includes the table ID, page number, is_dirty status, is_pinned status, and information about the Least Recently Used (LRU) list. When a user accesses a frame, the is_pinned status is set to "1", and that buffer is removed from the LRU list. When the buffer pool becomes full, the Buffer Manager identifies the LRU buffer and removes it. The buffer writes the frame to disk when the page's is_dirty status is set to "1".

## Functions

1. **buffer_check**: This function checks if the requested page exists in the buffer pool. If the page exists, it returns its index; otherwise, it returns "-1".

2. **buffer_unpin**: This function unpins the page after reading or writing. It decreases the is_pinned status, and if the is_pinned status is "0", it adds the page to the LRU list.

3. **buffer_alloc_page**: This function assists in allocating a page. It calls `file_alloc_page` to obtain a new page number. Then, it allocates a buffer space from the LRU list for the newly allocated page. Finally, it returns a pointer to that buffer.

4. **buffer_read_page**: This function helps in reading pages. It first checks if the page is in the buffer pool. If the page is not in the buffers, it reads the page from disk and adds it to the buffer. In this situation, it retrieves a buffer space from the LRU list. If the buffer is "dirty", it writes it.

5. **buffer_write_page**: This function is called when a user wants to update the page. It simply changes the is_dirty status to "1" and unpins the page. This page will be written when its buffer is used for another page.

6. **buffer_init**: This function initializes the buffer pool. It allocates space for the buffer in memory and sets the initial values.

7. **buffer_clear**: This function clears the buffer pool. It checks all buffers to see if their is_dirty status is "1" and writes that page to disk. Then it frees buffer spaces.

