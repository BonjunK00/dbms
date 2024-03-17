#ifndef __TRX_H__
#define __TRX_H__

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bpt.h"

typedef struct lock_t lock_t;
typedef struct lock_table_t lock_table_t;

// Struct of lock object.
struct lock_t {
    int64_t key;
    int lock_mode;

    lock_t *prev;
    lock_t *next;
    lock_table_t *sentinel;
    int flag;

    int trx_id;
    lock_t *next_trx_lock;

    char *original_value;
};

// Struct of lock table elements.
struct lock_table_t {
    int64_t table_id;
    pagenum_t pagenum;
    lock_t *tail;
    lock_t *head;
};

/* APIs for Lock Manager */
void lock_update_wait_for(lock_t *new_lock);
int lock_check_deadlock(int trx_id);
int init_lock_table();
lock_t *lock_acquire(int64_t table_id, pagenum_t pagenum, int64_t key, int trx_id, int lock_mode);
int lock_release(lock_t *lock_obj);

/* APIs for Transaction Manager */
int trx_insert(int trx_id, lock_t *new_lock);
int trx_find(int trx_id);
int trx_abort(int trx_id);
int trx_begin(void);
int trx_commit(int trx_id);

#endif /* __TRX_H__ */
