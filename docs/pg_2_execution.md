\page pg_execution Execution

# Write Thread
All write actions are encapsulated in `worker_write()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer. After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "non-blocking master").

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (see lines !!!). A detailed explanation can be found in \ref pg_synchronization "synchronization". Below are the steps taken by the worker thread in executing a write command.
1. The file is accessed using a `FILE` pointer `target_file`. The filepath is passed using the thread arguments. We use `r_simulate_access()` to introduce the specified delay.
2. Write the user input to the targeted file (line !!!). We sleep 25ms per character in the user input (line !!!). 
    - We use the append mode. This creates the file if it does not exist. 


# Read Thread
All read actions are encapsulated in `worker_read()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer. After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "non-blocking master").

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (see lines !!!). A detailed explanation can be found in \ref pg_synchronization "synchronization". Below are the steps taken by the worker thread in executing a read command.
1. The file-to-be-read is accessed using a `FILE` pointer `from_file`. The filepath is passed using the thread arguments. We use `r_simulate_access()` to introduce the specified delay.
2. Check if the `from_file` exists
    - **FILE DOES NOT EXIST**
        -# Acquire *global read.txt file lock*
        -# Record "FILE DNE" to read.txt
        -# Release *global read.txt file lock*
    - **FILE EXISTS**
        -# Acquire *global read.txt file lock*
        -# Access read.txt using pointer `to_file`
        -# Write the corresponding record header to read.txt
        -# Dump `from_file` contents to read.txt
        -# Release *global read.txt file lock*

# Empty Thread
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (see lines !!!). A detailed explanation can be found in \ref pg_synchronization "synchronization". Below are the steps taken by the worker thread in executing an empty command.
1. The file-to-be-read is accessed using a `FILE` pointer `from_file`. The filepath is passed using the thread arguments. We use `r_simulate_access()` to introduce the specified delay.
2. Check if the `from_file` exists
    - **FILE DOES NOT EXIST**
        -# Acquire *global empty.txt file lock*
        -# Record "FILE ALREADY EMPTY" to empty.txt
        -# Release *global empty.txt file lock*
    - **FILE EXISTS**
        -# Acquire *global empty.txt file lock*
        -# Access empty.txt using pointer `to_file`
        -# Write the corresponding record header to empty.txt
        -# Dump `from_file` contents to empty.txt
        -# Empty/erase `from_file` contents by opening file in write mode then immediately closing it (see `empty_file()`)
        -# Release *global empty.txt file lock*
