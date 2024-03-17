#include <stdint.h>
#include "buffer.h"

struct buffer_pool buffer;

// Initialize Mutex and conditial variable
pthread_mutex_t buffer_manager_latch = PTHREAD_MUTEX_INITIALIZER;

// Check the page and return if it exists.
int buffer_check(int64_t table_id, pagenum_t pagenum) {
    int i;
    for(i = 0; i < buffer.num_buf; i++) {
        if(buffer.list[i].table_id == table_id && buffer.list[i].pagenum == pagenum)
            return i;
    }
    return -1;
}

// Unpin the page.
void buffer_page_unlatch(struct page_t * page) {
    struct buffer_t * unpin_page = (struct buffer_t *)page;

    pthread_mutex_unlock(&unpin_page->page_latch);

    unpin_page->next = buffer.LRU_end;
    unpin_page->prev = buffer.LRU_end->prev;
    buffer.LRU_end->prev->next = unpin_page;
    buffer.LRU_end->prev = unpin_page;
}

// Allocate a new page and return the page.
struct page_t * buffer_alloc_page(int64_t table_id, pagenum_t * ret_pagenum) {
    // Buffer Manager Latch
    pthread_mutex_lock(&buffer_manager_latch);

    pagenum_t new_pagenum = file_alloc_page(table_id);
    *ret_pagenum = new_pagenum;

    // Find LRU page
    struct buffer_t * new_page = buffer.LRU_begin->next;

    // Update LRU list.
    buffer.LRU_begin->next = new_page->next;
    buffer.LRU_begin->next->prev = buffer.LRU_begin;

    // Write the dirty page.
    if(new_page->is_dirty == 1)
        file_write_page(new_page->table_id, new_page->pagenum, (struct page_t *)new_page->frame);

    // Fetch the on-disk page to the buffer pool.
    file_read_page(table_id, new_pagenum, (struct page_t *)new_page);
    new_page->table_id = table_id;
    new_page->pagenum = new_pagenum;
    new_page->is_dirty = 0;

    // Update free page number of the header page on buffer
    int buf_index;
    buf_index = buffer_check(table_id, 0x0);
    if (buf_index >= 0) {
        ((header_page_t *)&buffer.list[buf_index])->free_page_num 
            = ((struct page_t *)new_page)->next_page;
    }

    // Page Latch
    int trylock_return = pthread_mutex_trylock(&new_page->page_latch);
    // Buffer Manager Unlatch
    pthread_mutex_unlock(&buffer_manager_latch);

    if (trylock_return != 0) {
        while (pthread_mutex_trylock(&new_page->page_latch) != 0);
    }

    return (struct page_t *)new_page;
}

// Free the page and add the page to LRU list
void buffer_free_page(struct page_t * page) {
    struct buffer_t * free_page = (struct buffer_t *)page;

    file_free_page(free_page->table_id, free_page->pagenum);

    free_page->is_dirty = 0;
    free_page->table_id = -1;
    free_page->pagenum = -1;

    // Add the page to LRU list
    buffer_page_unlatch(page);
}

// Read an on-disk page into a buffer frame.
// Replace the least recently used buffer page and fetch one.
struct page_t * buffer_read_page(int64_t table_id, pagenum_t pagenum) {
    // Buffer Manager Latch
    pthread_mutex_lock(&buffer_manager_latch);

    // Page already exists in the buffer pool. 
    int buf_index;
    buf_index = buffer_check(table_id, pagenum);
    if (buf_index >= 0) {
        // Update LRU list if the page is in it.
        buffer.list[buf_index].prev->next = buffer.list[buf_index].next;
        buffer.list[buf_index].next->prev = buffer.list[buf_index].prev;

        // Page Latch
        int trylock_return = pthread_mutex_trylock(&buffer.list[buf_index].page_latch);
        // Buffer Manager Unlatch
        pthread_mutex_unlock(&buffer_manager_latch);

        if (trylock_return != 0) {
            while (pthread_mutex_trylock(&buffer.list[buf_index].page_latch) != 0);
    }

        return (struct page_t *)&buffer.list[buf_index];
    }

    // Find LRU page
    struct buffer_t * new_page = buffer.LRU_begin->next;

    
    // Update LRU list.
    buffer.LRU_begin->next = new_page->next;
    buffer.LRU_begin->next->prev = buffer.LRU_begin;

    // Page Latch
    int trylock_return = pthread_mutex_trylock(&new_page->page_latch);
    // Buffer Manager Unlatch
    pthread_mutex_unlock(&buffer_manager_latch);

    if (trylock_return != 0) {
        while (pthread_mutex_trylock(&new_page->page_latch) != 0);
    }

    // Write the dirty page.
    if(new_page->is_dirty == 1)
        file_write_page(new_page->table_id, new_page->pagenum, (struct page_t *)new_page->frame);

    // Fetch the on-disk page to the buffer pool.
    file_read_page(table_id, pagenum, (struct page_t *)new_page);
    new_page->table_id = table_id;
    new_page->pagenum = pagenum;
    new_page->is_dirty = 0;

    return (struct page_t *)new_page;
}

// Change buffer page is_dirty status to 1.
void buffer_write_page(struct page_t * dirty_page) {
    ((struct buffer_t *)dirty_page)->is_dirty = 1;
    buffer_page_unlatch(dirty_page);
}

// Initalizing.
void buffer_init(int num_buf) {
    // Allocate memory to buffer pool list.
    buffer.list = (struct buffer_t *)malloc(num_buf * sizeof(struct buffer_t));
    buffer.LRU_begin = (struct buffer_t *)malloc(sizeof(struct buffer_t));
    buffer.LRU_end = (struct buffer_t *)malloc(sizeof(struct buffer_t));
    if (buffer.list == NULL || buffer.LRU_begin == NULL || buffer.LRU_end == NULL) {
        perror("Buffer pool list creation.");
        exit(EXIT_FAILURE);
    }

    int i;
    for(i = 0; i < num_buf; i++) {
        buffer.list[i].is_dirty = 0;
        buffer.list[i].page_latch = PTHREAD_MUTEX_INITIALIZER;
        buffer.list[i].table_id = -1;
        buffer.list[i].pagenum = -1;

        if(i == 0)
            buffer.list[i].prev = buffer.LRU_begin;
        else
            buffer.list[i].prev = &buffer.list[i - 1];

        if(i == num_buf - 1)
            buffer.list[i].next = buffer.LRU_end;
        else
            buffer.list[i].next = &buffer.list[i + 1];
    }

    buffer.num_buf = num_buf;
    buffer.LRU_begin->prev = NULL;
    buffer.LRU_end->next = NULL;
    buffer.LRU_begin->next = &buffer.list[0];
    buffer.LRU_end->prev = &buffer.list[num_buf - 1];
}

// Clear
void buffer_clear() {
    // Write pages if it is dirty.
    int i;
    for(i = 0; i < buffer.num_buf; i++) {
        if(buffer.list[i].is_dirty == 1)
            file_write_page(buffer.list[i].table_id, buffer.list[i].pagenum, (struct page_t *)&buffer.list[i]);
    }

    free(buffer.list);
    free(buffer.LRU_begin);
    free(buffer.LRU_end);
    buffer.num_buf = 0;
}
