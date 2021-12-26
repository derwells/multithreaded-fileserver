\page pg_synchronization Synchronization

# Overview
The program ensures synchronization by enforcing a thread run order per target file. This is done using (1) a linked list of file metadata `tracker` and (2) a hand-over-hand locking scheme for threads targeting the same file.

# Metadata Tracker `tracker`
Linked list of existing target files and their corresponding `fmeta`. Indexed using filepath.

This is not threadsafe - hence it is non-blocking. Only the master thread accesses this data structure.

## Tracker node `lnode_t`
Key, value mapping for filepath to corresponding file metadata.

## Metadata `fmeta`
Key component of hand-over-hand locking and building worker threads. This keeps track of the most recent `out_lock` in `fmeta.recent_lock`.

# Hand-over-hand locking
Every type of worker thread uses hand-over-hand locking to ensure execution order. Every spawned thread will have their own `in_lock` and `out_lock`. The `in_lock` guards execution of the whole thread. The `out_lock` is released once the thread is finished - allowing the next thread to run. (see `args_t` for a description of thread arguments).

Worker threads are built so that their `in_lock` is the most recent thread's `out_lock` (tracked by `fmeta.recent_lock`). A worker thread's `in_lock` is unlocked **if and only if** any of the following conditions are met:
 -# It the first thread of a target file. There have be en no previous commands to the target file. (see lines !!!)
 -# A previous thread is finished executing (see lines !!!)
Condition (1) is expounded upon later in this chapter.


## Adding new command

# Testing

