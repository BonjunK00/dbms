#include <gtest/gtest.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string>

#include "db.h"

#define TRANSFER_THREAD_NUMBER (30)
#define SCAN_THREAD_NUMBER (1)

#define TRANSFER_COUNT (100000)
#define SCAN_COUNT (1)

#define RECORD_NUMBER (100)
#define INITIAL_MONEY (1000)
#define MAX_MONEY_TRANSFERRED (10)
#define SUM_MONEY (RECORD_NUMBER * INITIAL_MONEY)
#define VALUE_SIZE (100)
#define TABLE_ID (1)

void int_to_char_array(int n, char* arr, int size) {
    for (int i = size - 1; i >= 0; i--) {
        arr[i] = (n % 10) + '0';
        n /= 10;
    }
}

int char_arr_to_int(char* arr, int size) {
    int res = 0;
    for (int i = 0; i < size; i++) {
        res *= 10;
        res += arr[i] - '0';
    }
    return res;
}

void char_arr_sum(int n, char* arr, int size) {
    int res = char_arr_to_int(arr, size);
    res += n;
    int_to_char_array(res, arr, size);
}

class DBTest : public ::testing::Test {
   protected:
    /*
     * NOTE: You can also use constructor/destructor instead of SetUp() and
     * TearDown(). The official document says that the former is actually
     * perferred due to some reasons. Checkout the document for the difference
     */
    DBTest() {
        init_db(10000);
        pathname = "DB_test.db", table_id = open_table(pathname.c_str());
    }

    ~DBTest() {
        if (table_id >= 0) {
            file_close_table_file();
            remove(pathname.c_str());
        }
    }

    int64_t table_id;      // file descriptor
    std::string pathname;  // path for the file
};


/*
 * This thread repeatedly transfers some money between accounts randomly.
 */
void* transfer_thread_func(void* arg) {
    int64_t source_record_id;
    int64_t destination_record_id;
    int money_transferred;
    int trx_id;
    char temp_value[VALUE_SIZE];
    int temp_money;
    uint16_t temp_val_size;

    trx_id = trx_begin();
    for (int i = 0; i < TRANSFER_COUNT; i++) {
        /* Decide the source account and destination account for transferring. */
        source_record_id = rand() % RECORD_NUMBER;
        destination_record_id = rand() % RECORD_NUMBER;

        /* Decide the amount of money transferred. */
        money_transferred = rand() % MAX_MONEY_TRANSFERRED;
        money_transferred = rand() % 2 == 0 ? (-1) * money_transferred : money_transferred;

        /* withdraw */
        db_find(TABLE_ID, source_record_id, temp_value, &temp_val_size, trx_id);
        char_arr_sum((-1) * money_transferred, temp_value, VALUE_SIZE);
        db_update(TABLE_ID, source_record_id, temp_value, VALUE_SIZE, &temp_val_size, trx_id);

        /* deposit */
        db_find(TABLE_ID, destination_record_id, temp_value, &temp_val_size, trx_id);
        char_arr_sum(money_transferred, temp_value, VALUE_SIZE);
        db_update(TABLE_ID, destination_record_id, temp_value, VALUE_SIZE, &temp_val_size, trx_id);
    }
    trx_commit(trx_id);

    return NULL;
}

/*
 * This thread repeatedly check the summation of all accounts.
 * Because the locking strategy is 2PL (2 Phase Locking), the summation must
 * always be consistent.
 */
void* scan_thread_func(void* arg) {
    long long sum_money;
    int trx_id;
    char temp_value[VALUE_SIZE];
    int temp_money;
    uint16_t temp_val_size;

    for (int i = 0; i < SCAN_COUNT; i++) {
        trx_id = trx_begin();
        sum_money = 0;

        /* Iterate all accounts and summate the amount of money. */
        for (int record_id = 0; record_id < RECORD_NUMBER; record_id++) {
            /* Summation. */
            db_find(TABLE_ID, record_id, temp_value, &temp_val_size, trx_id);
            sum_money += char_arr_to_int(temp_value, VALUE_SIZE);
        }
        trx_commit(trx_id);

        /* Check consistency. */
        EXPECT_EQ(sum_money, SUM_MONEY);
    }

    return NULL;
}

TEST_F(DBTest, MultiTransactionTest) {
    pthread_t transfer_threads[TRANSFER_THREAD_NUMBER];
    pthread_t scan_threads[SCAN_THREAD_NUMBER];
    srand(time(NULL));

    // Initialize accounts.
    char input_val[VALUE_SIZE];
    for (int record_id = 0; record_id < RECORD_NUMBER; record_id++) {
        // Set the end of the value differently
        int_to_char_array(INITIAL_MONEY, input_val, VALUE_SIZE);

        db_insert(table_id, record_id, input_val, VALUE_SIZE);
    }

    // Transfer thread create
    for (int i = 0; i < TRANSFER_THREAD_NUMBER; i++) {
        pthread_create(&transfer_threads[i], 0, transfer_thread_func, NULL);
    }
    // Transfer thread join
    for (int i = 0; i < TRANSFER_THREAD_NUMBER; i++) {
        pthread_join(transfer_threads[i], NULL);
    }
    // Scan thread create
    for (int i = 0; i < SCAN_THREAD_NUMBER; i++) {
        pthread_create(&scan_threads[i], 0, scan_thread_func, NULL);
    }
    // Scan thread join
    for (int i = 0; i < SCAN_THREAD_NUMBER; i++) {
        pthread_join(scan_threads[i], NULL);
    }
}