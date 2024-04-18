# DBMS Project

## Description
This project is a comprehensive implementation of core database management components, including the File Manager, Buffer Manager, Index Manager, Lock Manager, and Transaction Manager.

- **File Manager**: Handles file storage, allowing the database to efficiently manage disk-based data.
- **Buffer Manager**: Manages the in-memory cache of database pages to reduce disk I/O operations, enhancing overall system performance.
- **Index Manager**: Implements indexing mechanisms to speed up data retrieval by maintaining organized data structures.
- **Lock Manager**: Ensures data consistency and supports concurrent access by managing locks on database objects.
- **Transaction Manager**: Oversees transactions, handling deadlock detection and initiating transaction abortion as necessary.


## Building and Running Instructions

### Building the Project
To configure the build system and build the project, follow these steps from the root directory of the project:

1. Configure the build system:
```
cmake -S . -B build
```
2. Move to the `build` directory:
```
cd build
```
3. Build the project:
```
cmake --build .
```


### Executable Files
The executable files generated through the build process will be located in the `build/bin` directory:

- `disk_based_db`: The executable for your `main.cc` file, which allows interaction with the database's main functionalities.
- `db_test`: The executable built from the test code, used to verify the correctness of the database functionalities.

### Running the Executables
To run the generated executables, use the following commands:

- To run the database:
```
./bin/disk_based_db
```
- To run the tests:
```
./bin/db_test
```
