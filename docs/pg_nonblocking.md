\page pg_nonblocking Non-blocking Master

# main()
Entrypoint `main()` initializes global variables and spawns the master thread.

# Master Thread
All master thread actions are found in `master()`. 

Note that the master thread also makes use of struct functions. Examples of these are `fmeta_init()` and `l_lookup()` (see !!!).

Below is a flowchart of how the master thread works.

![image](master-flowchart.png)

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

Although updating the file metadata `fmeta` and thread args `args_t` involve 
