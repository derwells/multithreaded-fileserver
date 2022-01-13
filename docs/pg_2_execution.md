\page pg_execution Execution

# Write Thread
All write actions are encapsulated in `worker_write()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer. The user input command is passed using `args_t.cmd` (`file_server.c:199-200`). After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "non-blocking master").

\snippet{lineno} docs/snippets.c worker_write

Line references will be interspersed in the explanation. For more, refer to the file documentation for `worker_write()` and associated functions. For ease of locating line-by-line explanations, references to the approriate functions will be made.

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (`file_server.c:202-227`). A detailed explanation can be found in \ref pg_synchronization "synchronization". The user input command is passed using `args_t.cmd`. Below are the steps taken by the worker thread in executing a write command.
-# [`file_server.c:209-211`] Access target file
    - We use the append mode. This creates the file if it does not exist.
-# [`file_server.c:214-217`] Error handling
-# [file_server.c:219-223] Write to target file (with sleep)
    - The file is accessed using a `FILE` pointer `target_file`. The filepath is passed using the thread arguments
    - [file_server.c:222] We use `r_simulate_access()` to introduce the specified delay.
-# [file_server.c:229-231] Free unneeded args and struct args
    - Free dynamically allocated memory


# Read Thread
All read actions are encapsulated in `worker_read()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer. The user input command is passed using `args_t.cmd` (`file_server.c:249-250`). After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "non-blocking master").

\snippet{lineno} docs/snippets.c worker_read

Line references will be interspersed in the explanation. For more, refer to the file documentation for `worker_read()` and associated functions.

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (`file_server.c:253-298`). A detailed explanation can be found in \ref pg_synchronization "synchronization". Below are the steps taken by the worker thread in executing a read command.

-# [`file_server.c:258-260`] Access target file
    - The file-to-be-read is accessed using a `FILE` pointer `from_file`. The filepath is passed using the thread arguments
    - We use `r_simulate_access()` to introduce the specified delay.
-# [`file_server.c:263-294`] Check if the target file (`from_file`) exists
    - [`file_server.c:269-274`] Target file does not exists; `read.txt` critical section (uses global lock)
        - [`file_server.c:270`] Open read.txt (see `open_read()`)
            - Wrapper for opening read.txt
            - `READ_TARGET` is `"read.txt"`
            - `READ_MODE` is `"a"`
        - file_server.c:272 Record "FILE DNE" to read.txt
            - See `FMT_READ_MISS` in `defs.h:36`
        - file_server.c:273 Close read.txt
    - file_server.c:281-291 Target file exists; `read.txt` critical section (uses global lock)
        - file_server.c:282 Access read.txt using pointer
        - file_server.c:285 Write the corresponding record header to read.txt (see `header2_cmd()`)
            - file_server.c:132-137 Writes a 2-input header
            - See `FMT_2CMD` in `defs.h:40`
        - file_server.c:288 Append file contents to read.txt (see `fdump()`)
            - file_server.c:168-170 Read contents per character
            - file_server.c:172 Place newline
        - file_server.c:290 Close read.txt
-# file_server.c:293 Close target file
-# [file_server.c:301-303] Free unneeded args and struct args
    - Free dynamically allocated memory

# Empty Thread
All empty actions are encapsulated in `worker_empty()`. Before the critical section begins, the thread arguments are typecasted into an `args_t` pointer (`file_server.c:321-322`). After the critical section, the thread arguments are freed except for `out_lock` (see \ref pg_nonblocking "non-blocking master").

\snippet{lineno} docs/snippets.c worker_empty

Line references will be interspersed in the explanation. For more, refer to the file documentation for `worker_empty()` and associated functions.

## Critical section
The critical section is bounded by `args_t.in_lock` and `args_t.out_lock` (`file_server.c:325-322`). A detailed explanation can be found in \ref pg_synchronization "synchronization". Below are the steps taken by the worker thread in executing a read command.

-# [`file_server.c:330-332`] Access target file
    - The file-to-be-read is accessed using a `FILE` pointer `from_file`. The filepath is passed using the thread arguments
    - We use `r_simulate_access()` to introduce the specified delay.
-# [`file_server.c:335-371`] Check if the target file (`from_file`) exists
    - [`file_server.c:341-346`] Target file does not exists; `empty.txt` critical section (uses global lock)
        - [`file_server.c:342`] Open empty.txt (see `open_read()`)
            - Wrapper for opening read.txt
            - `EMPTY_TARGET` is `"empty.txt"`
            - `EMPTY_MODE` is `"a"`
        - file_server.c:344 Record "FILE ALREADY EMPTY" to empty.txt
            - See `FMT_EMPTY_MISS` in `defs.h:38`
        - file_server.c:345 Close empty.txt
    - file_server.c:353-363 Target file exists
        - `empty.txt` critical section (uses global lock)
            - file_server.c:354 Access empty.txt using pointer
            - file_server.c:356 Write the corresponding record header to empty.txt (see `header2_cmd()`)
                - file_server.c:132-137 Writes a 2-input header
                - See `FMT_2CMD` in `defs.h:40`
            - file_server.c:359 Append file contents to empty.txt (see `fdump()`)
                - file_server.c:168-170 Read contents per character
                - file_server.c:172 Place newline
            - file_server.c:362 Close empty.txt
        - file_server.c:366-367 Empty target file then close it (see `empty_file()`)
            - file_server.c:184 Close target file
        - file_server.c:370: Sleep 7-10 seconds after appending and emptying (see `r_sleep_range`)
            - file_server.c:118 Translate random number to range
                - Random number from `rng()` - the provided generator
            - file_server.c:120 Microsecond sleep
-# [file_server.c:377-379] Free unneeded args and struct args
    - Free dynamically allocated memory

