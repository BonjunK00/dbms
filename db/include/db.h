#ifndef __DB_H_
#define __DB_H_

#include <cstdint>
#include <vector>
#include "bpt.h"
#include "trx.h"

// Open an existing database file or create one if not exist.
int64_t open_table(const char* pathname);

// Insert a record to the given table.
int db_insert(int64_t table_id, int64_t key, const char* value,
uint16_t val_size);

// Find a record with the matching key from the given table.
int db_find(int64_t table_id, int64_t key, char* ret_val,
uint16_t* val_size, int trx_id);

// Update a record with the matching key from the given table.
int db_update(int64_t table_id, int64_t key, char* value, 
                uint16_t new_val_size, uint16_t* old_val_size, int trx_id);

// Delete a record with the matching key from the given table.
int db_delete(int64_t table_id, int64_t key);

// Find records with a key between the range: 𝑏𝑒𝑔𝑖𝑛_𝑘𝑒y ≤ 𝑘𝑒𝑦 ≤ 𝑒𝑛𝑑_𝑘𝑒𝑦
int db_scan(int64_t table_id, int64_t begin_key, int64_t end_key, 
                std::vector<int64_t>* keys, std::vector<char*>* values,
                std::vector<uint16_t>* val_sizes);

// Initialize the database system.
int init_db(int num_buf);

// Shutdown the database system.
int shutdown_db();

#endif  // DB_H_
