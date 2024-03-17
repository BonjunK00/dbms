#include <stdio.h>
#include "db.h"

// Open an existing database file or create one if not exist.
int64_t open_table(const char* pathname){
    return file_open_table_file(pathname);
}

// Insert a record to the given table.
int db_insert(int64_t table_id, int64_t key, const char* value,
uint16_t val_size) {
    return insert(table_id, key, value, val_size);
}

// Find a record with the matching key from the given table.
int db_find(int64_t table_id, int64_t key, char* ret_val,
uint16_t* val_size, int trx_id) {
    // Trx is already aborted
    if(trx_find(trx_id) == -1)
        return -1;

    int i = 0;
    leaf_node * c;
    lock_t *acquired_lock;

    pagenum_t leaf_pagenum = find_leaf( table_id, key );

    if(leaf_pagenum == -1 || leaf_pagenum == 0)
        return -1;

    c = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);

    for (i = 0; i < c->num_keys; i++)
        if (read_leaf_key(c, i) == key) break;

    // Can't find matching key
    if (i == c->num_keys) {
        buffer_page_unlatch((struct page_t *)c);
        return -1;
    }
    buffer_page_unlatch((struct page_t *)c);

    // Can find the matching key, request shared lock
    acquired_lock = lock_acquire(table_id, leaf_pagenum, key, trx_id, 0);

    if(acquired_lock == NULL)
        return -1;

    c = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);

    *val_size = read_leaf_val_size(c, i);
    read_leaf_value(c, ret_val, i);

    buffer_page_unlatch((struct page_t *)c);
    
    return 0;
}

// Update a record with the matching key from the given table.
int db_update(int64_t table_id, int64_t key, char* value, uint16_t new_val_size,
                uint16_t* old_val_size, int trx_id) {
    // Trx is already aborted
    if(trx_find(trx_id) == -1)
        return -1;

    int i = 0;
    leaf_node * c;
    lock_t *acquired_lock;
    char* original_value;

    pagenum_t leaf_pagenum = find_leaf( table_id, key );

    if(leaf_pagenum == -1 || leaf_pagenum == 0)
        return -1;

    c = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);

    for (i = 0; i < c->num_keys; i++)
        if (read_leaf_key(c, i) == key) break;

    // Can't find matching key
    if (i == c->num_keys) {
        buffer_page_unlatch((struct page_t *)c);
        return -1;
    }
    buffer_page_unlatch((struct page_t *)c);

    // Can find the matching key, request exclusive lock
    acquired_lock = lock_acquire(table_id, leaf_pagenum, key, trx_id, 1);
    //printf("Update 2\n");
    if(acquired_lock == NULL)
        return -1;
    
    c = (leaf_node *)buffer_read_page(table_id, leaf_pagenum);

    // Backup the original value for using when the trx is aborted
    if(acquired_lock->original_value == NULL) {
        original_value = (char *)malloc(read_leaf_val_size(c, i) * sizeof(char));
        read_leaf_value(c, original_value, i);
        acquired_lock->original_value = original_value;
    }

    *old_val_size = read_leaf_val_size(c, i);
    write_leaf_val_size(c, new_val_size, i);
    write_leaf_value(c, value, new_val_size, i);

    buffer_page_unlatch((struct page_t *)c);

    return 0;
}

// Delete a record with the matching key from the given table.
int db_delete(int64_t table_id, int64_t key) {
    return bpt_delete(table_id, key);
}

// Find records with a key between the range: 𝑏𝑒𝑔𝑖𝑛_𝑘𝑒𝑦 ≤ 𝑘𝑒𝑦 ≤ 𝑒𝑛𝑑_𝑘𝑒𝑦
int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, 
                std::vector<int64_t>* keys, std::vector<char*>* values,
                std::vector<uint16_t>* val_sizes) {
    return find_range(table_id, begin_key, end_key, keys, values, val_sizes);
}

// Initialize the database system.
int init_db(int num_buf) {
    file_init_table_list(20);
    buffer_init(num_buf);
    init_lock_table();
    return 0;
}

// Shutdown the database system.
int shutdown_db() {
    buffer_clear();
    file_close_table_file();
    return 0;
}
