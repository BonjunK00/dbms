#include "trx.h"

// Global transaction id.
int global_trx_id;

// For using pair as a hash key.
struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
    }
};


// Lock Table
std::unordered_map <std::pair<int64_t, int64_t>, lock_table_t, pair_hash> lock_table;
// Transaction Table
std::unordered_map<int, lock_t *> trx_table;


// Initialize Mutex and conditial variable
pthread_mutex_t lock_manager_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t trx_manager_latch = PTHREAD_MUTEX_INITIALIZER;

// Wait-for graph
std::unordered_map <int, std::vector<int>> wait_for;

// For checking cycle
std::unordered_set <int> visited, trc;

// Initialize the lock table.
int init_lock_table() {
    lock_table.clear();
    return 0;
}

void lock_update_wait_for(lock_t *new_lock) {
    lock_t *cur_lock;
    int prev_lock_exist, trx_exist, i, is_shared_lock;

    prev_lock_exist = 0;
    is_shared_lock = !new_lock->lock_mode;
    cur_lock = new_lock->prev;
    while (cur_lock != NULL) {
        // If the record key is different
        if (cur_lock->key != new_lock->key) {
            cur_lock = cur_lock->prev;
            continue;
        }

        if (cur_lock->lock_mode == 1) is_shared_lock = 0;

        // Check the trx id already exists in the wait-for graph
        trx_exist = 0;
        for (i = 0; i < wait_for[new_lock->trx_id].size(); i++) {
            if (wait_for[new_lock->trx_id][i] == cur_lock->trx_id) {
                trx_exist = 1;
                break;
            }
        }
        if (!trx_exist && !is_shared_lock) wait_for[new_lock->trx_id].push_back(cur_lock->trx_id);

        // Set flag to 1 when the previous acquired lock mode is read
        if (prev_lock_exist == 0 && new_lock->lock_mode == 0 && cur_lock->lock_mode == 0 && cur_lock->flag == 1) {
            new_lock->flag = 1;
        }
        prev_lock_exist = 1;

        cur_lock = cur_lock->prev;
    }
    // Set flag to 1 when there are not any lock object that has the same record key
    if (prev_lock_exist == 0) new_lock->flag = 1;
}

int lock_check_deadlock(int trx_id) {
    if (trc.find(trx_id) != trc.end()) return 1;
    if (visited.find(trx_id) != visited.end()) return 0;
    trc.insert(trx_id);

    int i;
    for (i = 0; i < wait_for[trx_id].size(); i++) {
        if (trx_find(wait_for[trx_id][i]) != 0) continue;
        if (lock_check_deadlock(wait_for[trx_id][i]) == 1) return 1;
    }
    visited.insert(trx_id);
    trc.erase(trx_id);
    return 0;
}

// Acquire a lock.
lock_t *lock_acquire(int64_t table_id, pagenum_t pagenum, int64_t key, int trx_id, int lock_mode) {
    //  Latch
    pthread_mutex_lock(&lock_manager_latch);

    lock_table_t *node;
    lock_t *new_lock, *cur_lock, *iso_check_lock;
    int is_not_isolated = 0;

    node = &lock_table[std::make_pair(table_id, pagenum)];
    node->table_id = table_id;
    node->pagenum = pagenum;

    // Check the lock object of the same trx already exists
    cur_lock = node->head;
    while (cur_lock != NULL) {
        if (cur_lock->key != key) {
            cur_lock = cur_lock->next;
            continue;
        }

        // Check whether the S lock is not alone
        if (cur_lock->trx_id != trx_id) {
            is_not_isolated = 1;
            cur_lock = cur_lock->next;
            continue;
        }

        // Change S lock to X lock if it is alone
        if (lock_mode == 1 && cur_lock->lock_mode == 0) {
            iso_check_lock = cur_lock->next;
            while (iso_check_lock != NULL) {
                if (iso_check_lock->key == key && iso_check_lock->flag == 1) is_not_isolated = 1;
                iso_check_lock = iso_check_lock->next;
            }
            if (is_not_isolated) {
                // Unlatch
                pthread_mutex_unlock(&lock_manager_latch);

                trx_abort(trx_id);
                return NULL;
            }
            cur_lock->lock_mode = 1;
        }

        // Unlatch
        pthread_mutex_unlock(&lock_manager_latch);
        return cur_lock;

        cur_lock = cur_lock->next;
    }

    // Create a new lock object
    new_lock = (lock_t *)malloc(sizeof(lock_t));
    if (new_lock == NULL) {
        perror("lock creation");
        exit(EXIT_FAILURE);
    }

    // Locate the new lock to the tail of the node and set flag
    new_lock->key = key;
    new_lock->lock_mode = lock_mode;
    new_lock->next = NULL;
    new_lock->prev = node->tail;
    new_lock->sentinel = node;
    new_lock->trx_id = trx_id;
    new_lock->original_value = NULL;
    trx_insert(trx_id, new_lock);

    if (node->tail != NULL) {
        node->tail->next = new_lock;
        new_lock->flag = 0;
    } else {
        node->head = new_lock;
        new_lock->flag = 1;
    }
    node->tail = new_lock;

    // Update wait-for graph
    lock_update_wait_for(new_lock);

    //  Check the deadlock
    trc.clear();
    visited.clear();
    if (lock_check_deadlock(trx_id) == 1) {
        //  Unlatch
        pthread_mutex_unlock(&lock_manager_latch);

        trx_abort(trx_id);
        return NULL;
    }
    //  Wait until flag changes to 1
    while (new_lock->flag == 0) pthread_cond_wait(&cond, &lock_manager_latch);

    //  Unlatch
    pthread_mutex_unlock(&lock_manager_latch);
    return new_lock;
};

// Release the lock.
int lock_release(lock_t *lock_obj) {
    //  Latch.
    pthread_mutex_lock(&lock_manager_latch);

    lock_table_t *node;
    lock_t *first_lock;
    int lock_mode;
    int64_t key;

    node = lock_obj->sentinel;
    lock_mode = lock_obj->lock_mode;
    key = lock_obj->key;

    // Update pointers
    if (lock_obj->prev == NULL) {
        node->head = lock_obj->next;
        if (lock_obj->next == NULL)
            node->tail = NULL;
        else
            lock_obj->next->prev = NULL;
    } else {
        lock_obj->prev->next = lock_obj->next;
        if (lock_obj->next == NULL)
            node->tail = lock_obj->prev;
        else
            lock_obj->next->prev = lock_obj->prev;
    }

    // Free the lock object
    free(lock_obj);

    // Find the first lock object that has the same key
    first_lock = node->head;
    while (first_lock != NULL) {
        if (first_lock->key == key) break;
        first_lock = first_lock->next;
    }

    //  Matcing key lock object does not exist
    if (first_lock == NULL) {
        // Lock object list is empty, erase the entry
        if (node->head == NULL) lock_table.erase(std::make_pair(node->table_id, node->pagenum));
        // Unlatch.
        pthread_mutex_unlock(&lock_manager_latch);
        return 0;
    }

    //  Awake the first lock object if its lock mode is exclusive
    if (first_lock->lock_mode == 1) {
        first_lock->flag = 1;
        pthread_cond_broadcast(&cond);
    }
    // Awake multiple lock objects if their lock mode is shared
    else if (lock_mode == 1) {
        while (first_lock != NULL) {
            if (first_lock->key == key && first_lock->lock_mode == 1) break;

            if (first_lock->key == key) first_lock->flag = 1;
            first_lock = first_lock->next;
        }
        pthread_cond_broadcast(&cond);
    }

    //  Unlatch.
    pthread_mutex_unlock(&lock_manager_latch);
    return 0;
}

int trx_insert(int trx_id, lock_t *new_lock) {
    // Latch.
    pthread_mutex_lock(&trx_manager_latch);

    if (trx_table.find(trx_id) == trx_table.end()) {
        // Unlatch.
        pthread_mutex_unlock(&trx_manager_latch);
        return -1;
    }

    new_lock->next_trx_lock = trx_table[trx_id];
    trx_table[trx_id] = new_lock;

    // Unlatch.
    pthread_mutex_unlock(&trx_manager_latch);
    return 0;
}

int trx_find(int trx_id) {
    // Latch.
    pthread_mutex_lock(&trx_manager_latch);

    if (trx_table.find(trx_id) == trx_table.end()) {
        // Unlatch.
        pthread_mutex_unlock(&trx_manager_latch);
        return -1;
    }
    // Unlatch.
    pthread_mutex_unlock(&trx_manager_latch);
    return 0;
}

int trx_abort(int trx_id) {
    leaf_node *abort_page;
    lock_t *cur_lock, *next_lock;
    int i;

    // Latch.
    pthread_mutex_lock(&trx_manager_latch);

    cur_lock = trx_table[trx_id];

    // Erase the trx
    trx_table.erase(trx_id);

    // Unlatch.
    pthread_mutex_unlock(&trx_manager_latch);

    while (cur_lock != NULL) {
        next_lock = cur_lock->next_trx_lock;
        // If the update of the page was conducted
        if (cur_lock->lock_mode == 1 && cur_lock->original_value != NULL) {
            // Read the page that I want to abort
            abort_page = (leaf_node *)buffer_read_page(cur_lock->sentinel->table_id, cur_lock->sentinel->pagenum);

            for (i = 0; i < abort_page->num_keys; i++)
                if (read_leaf_key(abort_page, i) == cur_lock->key) break;

            // Rollback the value
            write_leaf_value(abort_page, cur_lock->original_value, read_leaf_val_size(abort_page, i), i);

            buffer_page_unlatch((struct page_t *)abort_page);
            free(cur_lock->original_value);
            cur_lock->original_value = NULL;
        }
        // Release the lock
        lock_release(cur_lock);
        cur_lock = next_lock;
    }
    return 0;
}

int trx_begin(void) {
    // Latch.
    pthread_mutex_lock(&trx_manager_latch);

    int trx_id = ++global_trx_id;
    trx_table[trx_id] = NULL;

    // Unlatch.
    pthread_mutex_unlock(&trx_manager_latch);
    return trx_id;
}
int trx_commit(int trx_id) {
    lock_t *cur_lock;
    lock_t *next_lock;

    // Latch.
    pthread_mutex_lock(&trx_manager_latch);

    // If trx has been aborted
    if (trx_table.find(trx_id) == trx_table.end()) {
        // Unlatch.
        pthread_mutex_unlock(&trx_manager_latch);
        return 0;
    }

    cur_lock = trx_table[trx_id];

    // Erase the trx
    trx_table.erase(trx_id);

    // Unlatch.
    pthread_mutex_unlock(&trx_manager_latch);

    while (cur_lock != NULL) {
        next_lock = cur_lock->next_trx_lock;
        if(cur_lock->original_value != NULL)
            free(cur_lock->original_value);
        lock_release(cur_lock);
        cur_lock = next_lock;
    }
    
    return trx_id;
}