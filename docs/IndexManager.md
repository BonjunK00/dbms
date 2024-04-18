# Index Manager

## Analyzing the Given B+ Tree Code

1. **Possible Call Paths of Insert/Delete Operations**

### Insert Operation

First, it checks if the key I want to insert exists in the tree by using `find`. If the key exists, the insert function finishes and just returns the root. Second, it creates a new record for the value by using `make_record`. Third, it checks if the tree already exists, and if it does not exist yet, creates a new tree by using `start_new_tree`. Fourth, it finds the location where the key should be inserted by using `find_leaf`. Fifth, finally, it inserts the key by using `insert_into_leaf` if there is room in the leaf node or by using `insert_into_leaf_after_splitting` if there is not.

### Delete Operation

First, it finds the key record and key leaf by using `find` and `find_leaf`. Then, it deletes the record using `delete_entry`. In the `delete_entry` function, it removes the record from the node using `remove_entry_from_node` and checks whether the number of keys is greater than the minimum number of keys. If the number of keys is still greater than the minimum number of keys, it just returns the root. If not, the function modifies the node structure. If the sum of the record-deleted node and its neighbor is smaller than the maximum number of keys, the function merges those nodes by using `coalesce_nodes`. If not, it redistributes nodes by using `redistribute_nodes`.

2. **Detail Flow of Structure Modifications (Split, Merge)**

### Split

Split occurs when there is not enough room while inserting the key. It creates a new leaf node and makes a temporary node to copy the node that needs to split with the newly inserted key. Then, it copies half of the temporary node's keys to the ordinary node and the other half to the new leaf node. Finally, it updates pointers of the ordinary node and the new node and inserts the key of the new node into its parent node.

### Merge

Merge occurs when the sum of the record-deleted node and its neighbor is smaller than the maximum number of keys. It copies all the keys of the record-deleted node to the neighbor node and then updates its pointers by separating two cases, either internal node or leaf node. Finally, it deletes the original node.

3. **Designs or Required Changes for Building an On-Disk B+ Tree**

The main point is changing the B+ tree in-memory node to an on-disk page node. The node struct should be saved on a page. If the tree wants to traverse the node, a file read function is required. And if the tree wants to update the node, a file write function is needed. The other point is that leaf nodes' shape must be changed. They should contain various lengths of records. So slot and record spaces are needed. To implement this, structure modification methods also should be changed. If the tree modifies the leaf node structure, slots and records must be moved to a new node.

## On-Disk B+ Tree

This structure is a B+ tree that implemented on-disk. Using it, to approach file pages becomes more efficient. It has seven operations to help deal with this B+ tree index.

1. **open_table**: This operation opens a file from disk and returns a table id. Using the table id, users can read and write that file without knowing the exact file descriptor or path.
2. **db_insert**: This operation writes keys and values in the leaf pages. If the leaf page is full, it makes a new leaf page and divides half of the keys and values to the new page. Then, it embeds that new leaf into its parent node. If the parent node is already full, it splits the parent node.
3. **db_find**: This operation finds the key and matched value and puts them into the parameter. It traverses the tree to find the exact key.
4. **db_delete**: This operation deletes the key and values. If there are fewer values in the leaf node than the threshold, it merges or redistributes that node (is not implemented yet).
5. **db_scan**: This operation scans the B+ tree from begin-key to end-key. It goes through the leaf node that contains the begin-key, then traverses to the next leaf node. If it reaches the end-key, it returns keys, value-sizes, and values.
6. **init_db**: This operation initializes the database management system.
7. **shutdown_db**: This operation shuts down the database management system.
