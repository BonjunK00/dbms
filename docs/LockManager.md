# Lock Manager

## Design

The Lock Manager provides Strict-2PL, which can be used for conflict-serializable schedules for transactions. The structure of the Lock Manager is a hash table, whose key is a combination of table ID and page ID, and the value of the table is a lock list. The lock list contains lock objects. Each lock object consists of a record ID and lock mode. This lock object blocks threads so that they can be executed serializably. The lock mode is explained below.

### Lock Mode

There are two modes in the Lock Manager - Shared Lock and Exclusive Lock. Shared Lock is for read operations, so it cannot change the value of records. Because of this reason, multiple threads can acquire the Shared Lock. However, Exclusive Lock can be acquired by just one thread because it can change the value, and it is used for write operations.

### Deadlock Detection

While the Lock Manager is executed, Deadlock can occur. Deadlock appears when the lock objects of multiple transactions wait for each other. This case must be handled, otherwise, they are waiting forever. So, when a new lock object is created, the Lock Manager updates a wait-for graph. Then, it traverses the wait-for graph for detecting cycles. If there is a cycle, the transaction that made that cycle is aborted.

### Abort and Rollback

Abort and Rollback are conducted when deadlock occurs. The Transaction Manager finds the lock object of the transaction that causes deadlock and releases them. In these processes, the Exclusive Lock must rollback the value to its original one. To do this, the lock object has a pointer to the original value.

## Functions

1. **init_lock_table**: It initializes the Lock Table and returns 0 if successful.

2. **lock_acquire**: It allocates a new lock object and appends it to the lock list. It first finds the location of the key, which is a pair of the input table ID and page ID. If there is no lock object in the lock list, it just appends the current object to the list and returns it. If there is already a lock object in the lock list, it sets the flag to 0 and puts the current object to sleep until the flag changes to 1 and returns that object when it wakes up. If the lock mode of the input lock object is Shared Lock and that of the previous lock object is Shared Lock too, set the flag to 1 immediately. It updates the wait-for graph and finds deadlock. If deadlock is observed, it aborts that transaction.

3. **lock_release**: It frees the input lock object and sets the flag of the next same-record object to 1. Then, it awakens the next object. If the lock mode of the next object is Shared Lock, it awakens the next Shared Lock object repeatedly.
