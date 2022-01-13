\page pg_nonblocking Non-blocking Master

For more line-by-line explanations, refer to the file documentation `main()`, `master()`, and associated functions.

# main()
Entrypoint `main()` initializes global variables and spawns the master thread. There will always at be at least 2 threads.

Here are more detailed steps
1. file_server.c:626-628 Initialize global mutexes
2. file_server.c:631-632 Initialize metadata tracker (see `l_init()`)
    - Sets `list_t.head` to NULL
3. file_server.c:635-636 Spawn master thread
4. file_server.c:639 Wait for master thread

# Master Thread
All master thread actions are found in `master()`. 

\snippet{lineno} docs/snippets.c master

Note that the master thread also makes use of struct-related functions. Examples of these are `fmeta_init()` and `l_lookup()`.

Figure \latexonly\ref{mtflcht}\endlatexonly is a flowchart of how the master thread works.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{master-flowchart.png}
	\caption{Master thread flowchart}
	\label{mtflcht}
\end{figure}
\endlatexonly


Here are more detailed steps
1. Parse and store user input in `command` struct
    - file_server.c:565-566 Store user input in dynamically allocated struct `command`; uses `get_command()`
        - file_server.c:396-399 Use scanf to read user commands. Handle file inputs by looping infinitely on EOF.
            - File inputs are useful for debugging
        - (`file_server.c:402-403,406-407,410-412`) Parse `scanf` using `strtok` then copy into `cmd`
2. Initialize thread arguments (see `args_init`)
    - file_server.c:569-570 Build thread arguments targs (see `args_init`)
        - file_server.c:483-484 Dynamically allocate and initialize new mutex for out_lock
        - file_server.c:487 Ensure next thread can't run immediately by locking out_lock
        - [`file_server.c:490-491`] Command information is deep-copied into other locations (e.g. file trackers `fmeta` and thread arguments `args_t`; see `command_copy()`)
3. Check file metadata tracker
    - This operation is a part of \ref pg_synchronization "*synchronization*"
    - See `l_lookup()`
        -  file_server.c:73-76 Compare file path as key; save lookup value and exit
    - file_server.c:578-601 Check if target file has been tracked
    - file_server.c:582-584 Metadata found, update respective fmeta.recent_lock 
        - file_server.c:583 Pass most recent `out_lock` as new thread's `in_lock`
    - file_server.c:587-598 Metadata not found, insert to file metadata tracker
        - file_server.c:590-591 Dynamically allocate and initialize new in_lock mutex
        - file_server.c:594-595 Dynamically allocate and initialize new file metadata fmeta (see `fmeta_init()`)
            - file_server.c:505 fmeta.path is the only initialization step
        - file_server.c:598 Insert new fmeta to the file metadata tracker(see `l_insert()`)
            - file_server.c:41 Dynamically allocate new node
            - file_server.c:43-56 Error handling
            - file_server.c:49-53 Build node and insert to linked list
4. Update file metadata
    - file_server.c:601 Whether newly-created or not, update target file's fmeta (see `fmeta_update()`)
        - file_server.c:519 Overwrite respective `fmeta.recent_lock` with new thread's `out_lock`
5. Spawn worker thread
    - file_server.c:604 Spawn worker thread (see `spawn_worker()`)
        - file_server.c:532 Use separate pointer for args_t.cmd for brevity
        - [`file_server.c:535-549`] Spawn appropriate thread based on action
            - [`file_server.c:538,541,544`] Pass thread arguments as void pointer
6. Record command in commands.txt
    - file_server.c:607 Record to commands.txt (see `command_record()`)
        - file_server.c:441-445 Build timestamp using system time 
            - file_server.c:447 Replace newline with null-terminator
        - file_server.c:448 Open commands.txt
        - file_server.c:451-454 Error handling if commands.txt does not exist
        - file_server.c:456-463 Write to commands.txt based on user input action
            - file_server.c:459 If recording write action, use 3 fprintf inputs
            - file_server.c:462 If not recording write action, use 2 fprintf inputs
7. Free `cmd`
    - file_server.c:612 Free cmd struct; copy of user input already in thread args

## Non-blocking Proof
It is never the case that the master thread gets blocked. The data structures used - `fmeta`, `command`, `list_t` - do not involve locks nor synchronization features.

Of note are the updating of the file metadata `fmeta` and thread args `args_t`. These involve shared locks - *pointers* to a lock used by worker threads. It should be emphasized that the locks themselves are not involved. The locks are simply "passed-along" with the use of pointers (`file_server.c:519,584`).

Figure \latexonly\ref{mlp2t}\endlatexonly summarizes the interaction between existing worker thread arugments (`t1`) and ones being built (`t2`).

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{master-lock-pointers-2threads.png}
	\caption{New thread t2, old thread t1}
	\label{mlp2t}
\end{figure}
\endlatexonly

Once a worker thread completes, it frees both `&in_lock` and `*in_lock` (in the Figure above, this is LOCK 0 - `*t1.in_lock`). By the time a worker thread completes, the `*in_lock` is not shared. It has reached the end of its life cycle. This ensures that `recent_lock` and `t2.in_lock` never point to invalid memory locations. Figure \latexonly\ref{mlp1t}\endlatexonly shows the lock pointer states when `t1` completesw.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{master-lock-pointers-1thread.png}
	\caption{t1 completes before t2}
	\label{mlp1t}
\end{figure}
\endlatexonly

Therefore, it is impossible for an existing worker thread to interfere with the master thread. The master thread can safely create worker threads.
