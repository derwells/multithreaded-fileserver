\page pg_synchronization Synchronization

# Overview
The program ensures synchronization by enforcing a thread run order per target file. This is done using (1) a linked list of file metadata `tracker` and (2) a hand-over-hand locking scheme for threads targeting the same file.

# Metadata Tracker
`tracker` is a linked list of existing target files and their corresponding `fmeta`. Indexed using filepath.

This is not threadsafe - hence it is non-blocking. Only the master thread accesses this data structure.

## Tracker node
`lnode_t` contains key, value mapping for filepath to corresponding file metadata.

## Metadata
`fmeta` is a key component of hand-over-hand locking and building worker threads. This keeps track of the most recent `out_lock` in `fmeta.recent_lock`.

# Hand-over-hand locking
## Rationale
Hand-over-hand locking, or HoH for short, is a simple way to overcome possible synchronization problems. Each file path has an associated "chain" of locks. This chain is built based on the arrival of worker threads. A worker thread can execute only if the thread before it unlocks a shared lock upon exiting. This shared lock is what prevents the worker thread from executing before the previousu thread. This principle ensures that threads targeting the same file path execute in the order they arrived.
Note that these "chain" of locks are only associated to a single file path. They are uninvolved in HoH locks for other files. This ensure that threads targeting different file paths are not mutually exclusive. Hence, they can run concurrently.

The only mutually exclusive operations between worker threads of different files the write operations to `read.txt` or `empty.txt`. The program uses global locks to guard these files.

## Explanation

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_overview.png}
	\caption{Synchronization overview}
	\label{overview}
\end{figure}
\endlatexonly

Every type of worker thread uses hand-over-hand locking to ensure execution order. Every spawned thread will have their own `in_lock` and `out_lock`. The `in_lock` guards execution of the whole thread. The `out_lock` is a shared lock that is released once the thread is finished, allowing the next thread to run. (see `args_t` for a description of thread arguments).

Worker threads are built so that their `in_lock` is the most recent thread's `out_lock` (tracked by `fmeta.recent_lock`). A worker thread's `in_lock` is unlocked **if and only if** any of the following conditions are met:
 -# It the first thread associated with a target file. There have been no previous commands to the target file. (see lines !!!)
 -# A previous thread is finished executing (see lines !!!)
Condition (1) is expounded upon later in this chapter.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=0.6]{sync_hoh.png}
	\caption{Hand-over-hand state}
	\label{hoh}
\end{figure}
\endlatexonly


In Figure \latexonly\ref{hoh}\endlatexonly, mutex a1 would have to be unlocked by cmd 1 before cmd 2 can execute (condition 2). However, mutex a0 would begin unlocked (condition 1).

## Adding new command
Say a new user input `cmd n+1` gets passed. 

*If the file tracker exists*, the program passes the most recent lock `fmeta.recent_lock` as the `in_lock` of `cmd n+1`. In Figure \latexonly\ref{hoh}\endlatexonly, the `in_lock` of `cmd n+1` would be mutex a10.

The program initializes a new mutex for `out_lock`. This overwrites the respective `fmeta.recent_lock`.

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
	\label{hoh_update_exists}
\end{figure}
\endlatexonly


# Addressed Synchronization Problems

## Race conditions
### Per target file
The HoH locking mechanism ensures that a thread can run only if
 - It is the first thread associated with a target file or
 - A previous thread released the shared lock (finished executing)

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

### To read.txt and empty.txt
The program uses global locks to guard writing to `read.txt` and `empty.txt`. These global locks are shared across every thread (not just per file path). Hence, no two threads can be writing to these output files at the same time. Worker threads will block in the middle of their execution and wait their turn.

These files do not have to be ordered - they are non-deterministic. Hence, it sufficies to ensure atomicity.

## Deadlocks
Nested locks are the most prone to causing deadlock errors. The only nested locks are `read.txt` and `empty.txt`. Per the suggestion of OSTEP, we ensure that all nested lock acesses follow the same ordering. When accessing these two global locks, we follow the order
 -# Get worker thread `in_lock`
 -# Get global lock
