#include "db.h"

#include <gtest/gtest.h>

#include <string>
#include <stdlib.h>
#include <time.h>


void int_to_char_arr(int n, char * arr, int size) {
  int i;
  for(int i = size - 1; i >= 0; i--){
    arr[i] = (n % 10) + '0';
    n /= 10;
  }
}


TEST(LeafNodeTest, CheckReadWrite) {
  leaf_node * leaf = make_leaf();

  write_leaf_key(leaf, 1234567890, 0);
  EXPECT_EQ(read_leaf_key(leaf, 0), 1234567890);

  write_leaf_val_size(leaf, 100, 0);
  EXPECT_EQ(read_leaf_val_size(leaf, 0), 100);

  write_leaf_offset(leaf, 100, 0);
  EXPECT_EQ(read_leaf_offset(leaf, 0), 100);

  char input_val[100] , output_val[100];
  uint16_t output_size;
  int i;

  for(i = 0; i < 100; i++)
    input_val[i] = ('a' + i) % 'z';
  write_leaf_value(leaf, input_val, 100, 0);
  read_leaf_value(leaf, output_val, 0);
  for(int j = 0; j < 100; j++)
    EXPECT_EQ(input_val[j], output_val[j]);
}

class DBTest : public ::testing::Test {
 protected:
  /*
   * NOTE: You can also use constructor/destructor instead of SetUp() and
   * TearDown(). The official document says that the former is actually
   * perferred due to some reasons. Checkout the document for the difference
   */
  DBTest() { init_db(10000); pathname = "DB_test.db", table_id = open_table(pathname.c_str()); }

  ~DBTest() {
    if (table_id >= 0) {
      file_close_table_file();
      remove(pathname.c_str());
    }
  }

  int64_t table_id;                // file descriptor
  std::string pathname;  // path for the file
};

// Insert one record and check it.
TEST_F(DBTest, CheckInsertionOnce) {
  char input_val[100], output_val[100];
  uint16_t output_size;
  int i;

  // Make an input record.
  for(i = 0; i < 20; i++)
    input_val[i] = ('a' + i);

  db_insert(table_id, 1, input_val, 20);
  find(table_id, 1, output_val, &output_size);

  // Compare input value with output value.
  EXPECT_EQ(output_size, 20);
  for(int j = 0; j < 20; j++)
    EXPECT_EQ(input_val[j], output_val[j]);
  
}


// Insert records with sequencial keys and check them.
TEST_F(DBTest, CheckSequencialOrderInsertion) {
  char input_val[50001][120], output_val[3000];
  int input_size = 45800, input_val_size = 100, i, j;
  uint16_t output_val_size;

  // Insert records.
  for(i = 1; i <= input_size; i++) {
    // Make values as "aaaaaa...".
    for(j = 0; j < input_val_size; j++)
      input_val[i][j] = 'a';
    // Set the end of the value differently
    int_to_char_arr(i, &input_val[i][input_val_size - 4], 4);

    db_insert(table_id, i, input_val[i], input_val_size);
  }

  // Check records.
  for(i = 1; i <= input_size; i++) {
    find(table_id, i, output_val, &output_val_size);
    //for(j = 0; j < input_val_size; j++)
      //EXPECT_EQ(input_val[i][j], output_val[j]);
  }
}


TEST_F(DBTest, CheckRandomInsertion) {
  char input_val[10001][120], output_val[120];
  int input_size = 10000, input_val_size = 100, i, j;
  uint64_t random_key[10001];
  uint16_t output_val_size;
  int already_exists = 0;

  // Initialize a random operation.
  srand((unsigned int)time(NULL));

  for(i = 0; i < input_size; i++) {
    // Make values as "aaaaaa...".
    for(j = 0; j < input_val_size; j++)
      input_val[i][j] = 'a';
    // Generate a random key
    random_key[i] = rand() % 100000;
    // Set the end of the value differently
    int_to_char_arr(i, &input_val[i][input_val_size - 6], 6);

    // Remove the duplicate key.
    already_exists = db_insert(table_id, random_key[i], input_val[i], input_val_size);
    if(already_exists == -1)
      i--;
    already_exists = 0;
  }

  // Check records.
  for(i = 0; i < input_size; i++) {
    find(table_id, random_key[i], output_val, &output_val_size);
    for(j = 0; j < input_val_size; j++) {
      if(input_val[i][j] != output_val[j])
      EXPECT_EQ(input_val[i][j], output_val[j]);
    }
  }
}


TEST_F(DBTest, CheckInsertScan) {
  char input_val[3010][120], output_val[120];
  int input_size = 3000, input_val_size = 100, i, j;
  uint16_t output_val_size;
  uint64_t begin = 2000, end = 2050;
  std::vector<int64_t> keys;
  std::vector< char* > values;
  std::vector<uint16_t> val_sizes;

  // Insert records.
  for(i = 1; i <= input_size; i++) {
    // Make values as "aaaaaa...".
    for(j = 0; j < input_val_size; j++)
      input_val[i][j] = 'a';
    // Set the end of the value differently.
    int_to_char_arr(i, &input_val[i][input_val_size - 4], 4);

    db_insert(table_id, i, input_val[i], input_val_size);
  }

  // Scan the tree and compare with its original values.
  db_scan(table_id, begin, end, &keys, &values, &val_sizes);
  for(i = 0; i < end - begin; i++) {
    for(j = 0; j < input_val_size; j++)
      EXPECT_EQ(input_val[i + begin][j], values[i][j]);
  }
}


// Insert and delete records with sequencial keys and check them.
TEST_F(DBTest, CheckSequencialOrderDeletion) {
  char input_val[30010][120], output_val[3000];
  int input_size = 30000, input_val_size = 100, i, j, exist;
  uint16_t output_val_size;

  // Insert records.
  for(i = 1; i <= input_size; i++) {
    // Make values as "aaaaaa...".
    for(j = 0; j < input_val_size; j++)
      input_val[i][j] = 'a';
    // Set the end of the value differently
    int_to_char_arr(i, &input_val[i][input_val_size - 4], 4);

    db_insert(table_id, i, input_val[i], input_val_size);
  }

  // Delete half of records.
  for(i = 1; i <= input_size / 2; i++) {
    db_delete(table_id, i);
  }

  // Check records.
  for(i = 1; i <= input_size; i++) {
    exist = find(table_id, i, output_val, &output_val_size);
    for(j = 0; j < input_val_size; j++) {
      if(i > input_size / 2)
        EXPECT_EQ(input_val[i][j], output_val[j]);
      else
        EXPECT_EQ(-1, exist);
    }
  }
}