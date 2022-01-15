\page pg_execution Execution

# Write Thread
All write actions are encapsulated in `worker_write()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer. The user input command is passed using `args_t.cmd` (file_server.c:208-209). After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "Non-blocking master").

\snippet{lineno} docs/snippets/snippets.c worker_write

Line references will be interspersed in the explanation. For more, refer to the file documentation for `worker_write()` and associated functions. For ease of locating line-by-line explanations, references to the approriate functions will be made.

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (file_server.c:212-237). A detailed explanation can be found in \ref pg_synchronization "Synchronization". The user input command is passed using `args_t.cmd`. Below are the steps taken by the worker thread in executing a write command.
-# file_server.c:218-220 Access target file
    - We use the append mode. This creates the file if it does not exist.
-# file_server.c:222-226 Error handling
-# file_server.c:229-233 Write to target file (with sleep)
    - See `ms2ts()`
        - file_server.c:25 Convert ms to s
        - file_server.c:28 Convert ms to ns
-# file_server.c:240-242 Free unneeded args and struct args
    - Does not include out_lock

# Read Thread
All read actions are encapsulated in `worker_read()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer. The user input command is passed using `args_t.cmd` (file_server.c:260-261). After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "Non-blocking master").

\snippet{lineno} docs/snippets/snippets.c worker_read

Line references will be interspersed in the explanation. For more, refer to the file documentation for `worker_read()` and associated functions.

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (file_server.c:264-311). A detailed explanation can be found in \ref pg_synchronization "Synchronization". Below are the steps taken by the worker thread in executing a read command.

-# file_server.c:269-271 Access target file
    - The file-to-be-read is accessed using a `FILE` pointer `from_file`. The filepath is passed using the thread arguments
    - We use `r_simulate_access()` to introduce the specified delay.
-# file_server.c:274-315 Check if the target file (from_file) exists and attempt to read it
    - file_server.c:280-291 Target file does not exists; `read.txt` critical section
        - file_server.c:283 Open read.txt (see `open_read()`)
            - Wrapper for opening read.txt
            - `READ_TARGET` is `read.txt`
            - `READ_MODE` is `a`
        - file_server.c:286 Record FILE DNE to read.txt
            - See `FMT_READ_MISS` in `defs.h:36`
        - file_server.c:289 Close read.txt
    - file_server.c:298-312  Runs if target file exists; `read.txt` critical section (uses global lock)
        - file_server.c:301 Open read.txt
        - file_server.c:304 Write the corresponding record header (see `header2_cmd()`)
            - file_server.c:141-146 Writes a 2-input header
            - See `FMT_2CMD` in `defs.h:40`
        - file_server.c:307 Append file contents to read.txt (see `fdump()`)
            - file_server.c:177-179 Read contents per character
            - file_server.c:182 Place newline
        - file_server.c:310 Close read.txt
-# file_server.c:315 Close target file
-# file_server.c:323-325 Free unneeded args and struct args
    - Does not include out_lock

# Empty Thread
All empty actions are encapsulated in `worker_empty()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer (file_server.c:343-344). After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "Non-blocking master").

\snippet{lineno} docs/snippets/snippets.c worker_empty

Line references will be interspersed in the explanation. For more, refer to the file documentation for `worker_empty()` and associated functions.

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (file_server.c:347-409). A detailed explanation can be found in \ref pg_synchronization "Synchronization". Below are the steps taken by the worker thread in executing a read command.

-# file_server.c:352-354 Access target file
    - The file-to-be-read is accessed using a `FILE` pointer `from_file`. The filepath is passed using the thread arguments
    - We use `r_simulate_access()` to introduce the specified delay.
-# file_server.c:357-406 Check if the target file (from_file) exists and attempt to read it
    - file_server.c:363-374 Runs if target does not exist
        - `empty.txt` critical section (uses global lock)
            - file_server.c:366 Open empty.txt (see `open_empty()`)
                - Wrapper for opening empty.txt
                - `EMPTY_TARGET` is `empty.txt`
                - `EMPTY_MODE` is `a`
            - file_server.c:369 Record FILE ALREADY EMPTY to empty.txt
                - See `FMT_EMPTY_MISS` in `defs.h:38`
            - file_server.c:372 Close empty.txt
    - file_server.c:381-395  Run if target exists
        - `empty.txt` critical section (uses global lock); 
            - file_server.c:384 Open empty.txt
            - file_server.c:386 Write the corresponding record header to empty.txt (see `header2_cmd()`)
                - file_server.c:141-146 Writes a 2-input header
                - See `FMT_2CMD` in `defs.h:40`
            - file_server.c:390 Append file contents to empty.txt(see `fdump()`)
                - file_server.c:177-179 Read contents per character
                - file_server.c:182 Place newline
            - file_server.c:393 Close empty.txt
        - file_server.c:398-399 Empty target file (see `empty_file()`)
            - file_server.c:193 Empty target file with mode `w`
            - file_server.c:194 Close target file
        - file_server.c:405: After appending to read.txt and emptying the target file, sleep after for random time in the range 7000ms-10000ms (see `r_sleep_range()`)
            - file_server.c:123 Translate random number to range
            - file_server.c:126-128 Convert ms to ns and sleep
-# file_server.c:412-414 Free unneeded args and struct args
    - Does not include out_lock

