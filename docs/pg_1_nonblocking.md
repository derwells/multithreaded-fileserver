\page pg_nonblocking Non-blocking Master

For more line-by-line explanations, refer to the file documentation `main()`, `master()`, and associated functions.

# main()
Entrypoint `main()` initializes global variables and spawns the master thread. There will always at be at least 2 threads.

Here are more detailed steps
1. [file_server.c:664] Set the random number generator seed
2. [file_server.c:667-669] Initialize global locks
3. [file_server.c:672-673] Initialize metadata tracker (see `l_init()`)
    - Sets `list_t.head` to NULL
4. [file_server.c:676-677] Spawn master thread
5. [file_server.c:680] Wait for master thread

# Master Thread
All master thread actions are found in `master()`. 

\snippet{lineno} docs/snippets/snippets.c master

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
    - [file_server.c:602-603] Store user input in dynamically allocated struct `command`; uses `get_command()`
        - [file_server.c:428-436] Reading user input into `inp` (max size 118 chars)
            - [file_server.c:435-436] Read command until newline
        - [file_server.c:438-439,442-443,446-448] Parse `inp` using `strtok` then copy into `cmd`
2. Initialize thread arguments (see `args_init()`)
    - [file_server.c:606-607] Build thread args targs (see `args_init()`)
        - [file_server.c:520-521] Dynamically allocate and initialize new mutex for out_lock
        - [file_server.c:524] Ensure next thread can't run immediately by locking out_lock
        - [file_server.c:527-528] Command information is deep-copied into other locations (e.g. file trackers `fmeta` and thread arguments `args_t`; see `command_copy()`)
3. Check file metadata tracker
    - This operation is a part of \ref pg_synchronization "*Synchronization*"
    - See `l_lookup()`
        - [file_server.c:84-89] Compare file path as key; save lookup value and exit
    - [file_server.c:615-636] Check if target file has been tracked
    - [file_server.c:619-621] Metadata found, update respective `fmeta.recent_lock`
        - [file_server.c:621] Pass most recent out_lock as new thread's `in_lock`
    - [file_server.c:624-635] Metadata not found, insert to file tracker
        - [file_server.c:627-628] Dynamically allocate and initialize new `in_lock` mutex
        - [file_server.c:631-632] Dynamically allocate and initialize new file metadata fmeta (see `fmeta_init()`)
            - [file_server.c:542] fmeta.path is the only initialization step
        - [file_server.c:635] Insert new fmeta to the file tracker(see `l_insert()`)
            - [file_server.c:51] Dynamically allocate new node 
            - [file_server.c:54-57] Error handling
            - [file_server.c:60-64] Build node and insert to linked list
4. Update file metadata
    - This operation is a part of \ref pg_synchronization "*Synchronization*"
    - [file_server.c:638] Whether newly-created or not, update target file's fmeta (see `fmeta_update()`)
        - [file_server.c:556] Overwrite target file's `fmeta.recent_lock` with new out_lock
5. Spawn worker thread
    - [file_server.c:641] Spawn worker thread (see `spawn_worker()`)
        - [file_server.c:572-586] Spawn appropriate worker thread depending on action
            - [file_server.c:575,578,581] Pass thread arguments as void pointer
6. Record command in commands.txt
    - [file_server.c:644] Record to commands.txt (see `command_record()`)
        - [file_server.c:477-481] Build timestamp using system time
            - [file_server.c:481] Replace newline with null-terminator
        - [file_server.c:484] Open commands.txt
        - [file_server.c:487-490] Error handling if commands.txt does not exist
        - [file_server.c:493-499] Record to commands.txt based on type of action
            - [file_server.c:495] If recording write action, use 3 inputs
            - [file_server.c:498] If not recording write action, use 2 inputs
7. Free `cmd`
    - [file_server.c:650] Free cmd struct; copy of user input already in thread args

## Non-blocking Proof
It is never the case that the master thread gets blocked. The data structures used - `fmeta`, `command`, `list_t` - do not involve locks nor synchronization features.

Of note are the updating of the file metadata `fmeta` and thread args `args_t`. These involve shared locks - *pointers* to a lock used by worker threads. It should be emphasized that the locks themselves are not involved. The locks are simply "passed-along" with the use of pointers (file_server.c:556,621).

Figure \latexonly\ref{mlp2t}\endlatexonly summarizes the interaction between existing worker thread arugments (`t1`) and ones being built (`t2`).

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{master-lock-pointers-2threads.png}
	\caption{New thread t2, old thread t1}
	\label{mlp2t}
\end{figure}
\endlatexonly

Once a worker thread completes, it frees both `&`in_lock`` and `*`in_lock`` (in the Figure above, this is LOCK 0 - `*t1.`in_lock``). By the time a worker thread completes, the `*`in_lock`` is not shared. It has reached the end of its life cycle. This ensures that `recent_lock` and `t2.`in_lock`` never point to invalid memory locations. Figure \latexonly\ref{mlp1t}\endlatexonly shows the lock pointer states when `t1` completesw.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{master-lock-pointers-1thread.png}
	\caption{t1 completes before t2}
	\label{mlp1t}
\end{figure}
\endlatexonly

Therefore, it is impossible for an existing worker thread to interfere with the master thread. The master thread can safely create worker threads.
