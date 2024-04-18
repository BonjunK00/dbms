# Transaction Manager

## Design

The Transaction Manager manages the information of transactions. It is structured as a hash table, where the key is the transaction ID and the value is a pointer to a lock object of that transaction. When a user wants to start a new transaction or finish it, the Transaction Manager offers these APIs. When deadlock is detected by the Lock Manager, the Transaction Manager aborts that transaction.

## Functions

1. **trx_begin**: It creates a new transaction ID and returns it.

2. **trx_commit**: It releases all lock objects of the input transaction ID and returns that ID. It returns 0 if the transaction is already aborted.
