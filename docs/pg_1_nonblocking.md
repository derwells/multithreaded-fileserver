\page pg_nonblocking Non-blocking Master

# main()
Entrypoint `main()` initializes global variables and spawns the master thread.

# Master Thread
All master thread actions are found in `master()`. 

Note that the master thread also makes use of struct functions. Examples of these are `fmeta_init()` and `l_lookup()` (see !!!).

Below is a flowchart of how the master thread works.

\image latex master-flowchart.png

Here are more detailed steps
1. Parse and store user input in `command` struct
    - Uses `get_command()` to read user input into a `command` struct
    - Command information is deep-copied into other locations (e.g. file trackers `fmeta` and thread arguments `args_t`)
2. Is filepath in metadata tracker? 
    - This operation is a part of \subpage pg_synchronization "synchronization".
    - **YES** Hand over recent lock
        - Pass most recent `out_lock` as new thread's `in_lock`
    - **NO** Create and add to metadata tracker. Initialize HoH locking
        - Create new unlocked mutex for new thread's `in_lock`
        - Dynamically create file metadata
        - Insert to tracker
3. Update file metadata
    - See wrapper function `fmeta_update()`
    - Points metadata's `recent_lock` to new thread's `out_lock`.
4. Spawn worker thread
    - See `spawn_worker()`
    - Comprised of if-else branches and `pthread_create()` invocations
5. Record command in command.txt
    - See `command_record()`

## Why master thread is non-blocking
It is never the case that the master thread gets blocked. The data structures used - `fmeta`, `command`, `list_t` - do not involve locks nor synchronization features.

Of note are the updating of the file metadata `fmeta` and thread args `args_t`. These involve *pointers* to a lock used by worker threads. It should be emphasized that the locks themselves are not involved. The locks simply "passed-along" with the use of pointers. 

The figure below summarizes the interaction between existing worker thread arugments (`t1`) and ones being built (`t2`).

\image latex master-lock-pointers-2threads.png

Once a worker thread completes, the only lock freed is the `in_lock` (in the diagram above, it is `t1.in_lock`). This ensures that `recent_lock` and `t2.in_lock` never point to invalid memory locations. The figure below shows what will happend if the thread of `t1` completes before the thread of `t2`.

\image latex master-lock-pointers-1thread.png

Therefore, it is impossible for an existing worker thread to interfere with the master thread. The master thread can safely create worker threads despite the lack of blocking and synchronization.
