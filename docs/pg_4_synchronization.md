\page pg_synchronization Synchronization

# Overview
The program ensures synchronization by enforcing a thread run order per target file. This is done using (1) a linked list of file metadata `tracker` (`file_server.c:20,631`; see \ref pg_nonblocking "Non-blocking Master") and (2) a hand-over-hand locking scheme for threads targeting the same file.

# Metadata Tracker
\snippet{lineno} docs/snippets.c global_vars

`tracker` is a linked list (`lnode_t`) of existing target files and their corresponding `fmeta`. Indexed using filepath.

This is not threadsafe. However, only the master thread accesses this data structure. Hence, it is non-blocking.

## Tracker node
`lnode_t` (`defs.h:108`) contains key, value mapping for filepath to corresponding file metadata.
 
## Metadata
`fmeta` (`defs.h:91`) is a key component of hand-over-hand locking and building worker threads. This keeps track of the most recent `out_lock` in `fmeta.recent_lock`.

# Hand-over-hand locking
## Rationale
Hand-over-hand locking, or HoH for short, is a simple way to overcome possible synchronization problems. Each file path has an associated "chain" of locks. This chain is built based on the arrival of worker threads. A worker thread can execute only if the thread before it unlocks a shared lock upon exiting. This shared lock is what prevents the worker thread from executing prematurely. This principle ensures that threads targeting the same file path execute in the order they arrived.

Note that these "chain" of locks are only associated to a single file path. They are uninvolved in hand-over-hand locks for other files. This ensure that threads targeting different file paths are not mutually exclusive. Hence, they can run concurrently.

The only mutually exclusive operations between worker threads of different files the write operations to `read.txt` or `empty.txt`. The program uses global locks to guard these files.

Line references will be interspersed in the explanation. A full line-by-line explanation can be found in the `file_server.c` section of the File Documentation. For ease of locating line-by-line explanations, references to the approriate functions will be made.

## Explanation

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_overview.png}
	\caption{Synchronization overview}
	\label{overview}
\end{figure}
\endlatexonly

Every type of worker thread uses hand-over-hand locking to ensure execution order. Every type of worker thread is passed mutex pointers `in_lock` and `out_lock`. The `*in_lock` guards execution of the whole thread (file_server.c:203,253,325). The `*out_lock` is a shared lock that is released once the thread is finished (file_server.c:227,298,374), allowing the next thread to run (see `args_t`).

\snippet{lineno} docs/snippets.c tracker_check

Worker threads are built such that their `*out_lock` begins locked (file_server.c:483-487). Their `*in_lock` is the `*out_lock` of the worker thread at the tail of the chain. The tail - or trailing - `*out_lock` is tracked by `fmeta.recent_lock` (file_server.c:584). Everytime a worker thread is built `fmeta.recent_lock` gets overwritten by the new `out_lock` (file_server.c:519).

A worker thread's `in_lock` is unlocked **if and only if** any of the following conditions are met
 -# It is the first thread associated with a target file (file_server.c:590-591)
 	This means there have been no previous commands to the target file
 -# The thread before it has finished executing
	This means the shared mutex has been unlocked

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_hoh.png}
	\caption{Hand-over-hand state}
	\label{hoh}
\end{figure}
\endlatexonly

See Figure \latexonly\ref{hoh}\endlatexonly for an example. Suppose that no worker thread has executed yet. Condition 1 ensures that mutex a0 would begin unlocked - allowing the chain of threads to begin. Condition 2 ensures that mutex a1 would have to be unlocked by cmd 1. This means cmd 1 has to execute before cmd 2.

## Adding new command
Say a new user input `cmd n+1` gets passed. 

*If the file tracker exists*, the program passes the most recent lock `fmeta.recent_lock` as the `in_lock` of `cmd n+1`. In Figure \latexonly\ref{hoh}\endlatexonly, the `in_lock` of `cmd n+1` would be mutex a10.

The program initializes a new mutex for `out_lock` (file_server.c:483-487). This overwrites the respective `fmeta.recent_lock`.

Figure \latexonly\ref{hoh_update_exists}\endlatexonly is what happens after the update.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_hoh_update_exists.png}
	\caption{Adding new command if tracker exists}
	\label{hoh_update_exists}
\end{figure}
\endlatexonly

*If the file tracker does not exist*, the program initializes a new mutex. This gets passed as the `in_lock` of `cmd n+1`. Figure \latexonly\ref{hoh_dne}\endlatexonly is what happens after the update.
\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_hoh_dne.png}
	\caption{Adding new command if tracker does not exist}
	\label{hoh_dne}
\end{figure}
\endlatexonly


# Addressed Synchronization Problems

## Race conditions
### Per target file
The HoH locking mechanism ensures that a thread can run only if
 -# It is the first thread associated with a target file (file_server.c:590-591)
 	This means there have been no previous commands to the target file
 -# The thread before it has finished executing
	This means the shared mutex has been unlocked


\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_hoh_rc_1.png}
	\caption{Race condition prevention}
	\label{rc_1}
\end{figure}
\endlatexonly


Say we have a pool of `n` worker threads waiting to run. In order, these are `cmd 1, 2, ... n` with associated locks `mutex a0, a1, ... a10`. The only entrypoint into the chain of locks is `mutex a0`. This is `cmd 1`'s `in_lock` - *the oldest waiting worker thread's `in_lock`*.

Suppose other threads aside from `cmd 1` execute first. Remember that all shared locks begin as locked. That means upon all these threads will eventually `yield()` and wait for the shared lock to open. Thus, `cmd 1` will defitely get to execute first.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_hoh_rc_2.png}
	\caption{`cmd 1` finishes}
	\label{rc_2}
\end{figure}
\endlatexonly


Once `cmd 1` finishes, the order is maintain as `cmd 2` is allowed to run next. Again, only `cmd 2`'s `in_lock` in unlocked. Other worker threads that were barred from execution in `cmd 1` are still barred while `cmd 2` is executing.

Through induction, we can see that the shared locks are opened one at a time.

### Accessing read.txt and empty.txt
The program uses global locks to guard writing to `read.txt` and `empty.txt` (file_server.c:269-274,281-291,341-346,353-363). These global locks are shared across every thread (not just per file path). Hence, no two threads can be writing to these output files at the same time. Worker threads will block in the middle of their execution and wait their turn.

These files do not have to be ordered - they are non-deterministic. Hence, it sufficies to ensure atomicity.

## Deadlocks
Nested locks prone to deadlock. The only nested locks are the global locks for `read.txt` and `empty.txt`. We ensure that all nested lock acesses follow the same ordering. The program always gets the `*in_lock` first before getting the global lock. In other words, we provide a *total ordering* on lock acquisition (OSTEP 32-7).

## Invalid memory locations
This portion addresses the validity of the freeing mechanism done by the worker threads. Figure \latexonly\ref{mlp2t}\endlatexonly summarizes the interaction between existing worker thread arugments (`t1`) and ones being built (`t2`).

Once a worker thread completes, it frees uneeded dynamic memory (file_server.c:229-231,301-303,377-379). This includes its `*in_lock` (in Figure \latexonly\ref{mlp2t}\endlatexonly, this is LOCK 0). This may seem like a problem because `*in_lock` starts out as a shared lock - it was an older thread's `*out_lock`. However by the time the current worker thread completes, the `*in_lock` is not shared anymore. The older worker thread would have already completed and freed its `out_lock` pointer. This ensures that `recent_lock` and `t2.in_lock` never point to invalid memory locations. Figure \latexonly\ref{mlp1t}\endlatexonly shows the lock pointer states when `t1` completes.
