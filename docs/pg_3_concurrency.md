\page pg_concurrency Concurrency

# Implementation
Each worker thread is associated with a target file. This allows for operations to a single file to be synchronized (see \ref pg_synchronization "*Synchronization*" for more info).

However, *worker threads associated with different files can run concurrently*. The blocked portion of each worker thread is guarded by an `in_lock` dependent on the target file. Thus, there is no mutual exclusivity between worker threads of a different file. The only mutually exclusive operations between worker threads of different files are the recording operations to `read.txt` or `empty.txt`.

# Proof
[FIX THIS]
Note the integrity test input below:

    write outputs/a.txt 1 2
    write outputs/a.txt 3 4
    read outputs/a.txt
    write outputs/a.txt 5 6
    write outputs/b.txt 1 2
    write outputs/b.txt 3 4
    empty outputs/a.txt
    read outputs/a.txt
    read outputs/a.txt
    write outputs/a.txt 7 8
    read outputs/b.txt
    read outputs/a.txt
    empty outputs/a.txt
    write outputs/b.txt 5 6
    empty outputs/b.txt
    read outputs/b.txt
    write outputs/a.txt 9 10
    write outputs/a.txt 11 12
    read outputs/b.txt
    write outputs/b.txt 7 8
    read outputs/b.txt
    empty outputs/b.txt
    write outputs/a.txt 13 14
    read outputs/a.txt
    write outputs/b.txt 11 12
    write outputs/b.txt 13 14
    read outputs/b.txt
    empty outputs/a.txt
    write outputs/b.txt 9 10
    empty outputs/b.txt
    write outputs/c.txt 1 2
    write outputs/c.txt 3 4
    read outputs/c.txt
    write outputs/c.txt 5 6
    write outputs/d.txt 1 2
    write outputs/d.txt 3 4
    empty outputs/c.txt
    read outputs/c.txt
    read outputs/c.txt
    write outputs/c.txt 7 8
    read outputs/d.txt
    read outputs/c.txt
    empty outputs/c.txt
    write outputs/d.txt 5 6
    empty outputs/d.txt
    read outputs/d.txt
    write outputs/c.txt 9 10
    write outputs/c.txt 11 12
    read outputs/d.txt
    write outputs/d.txt 7 8
    read outputs/d.txt
    empty outputs/d.txt
    write outputs/c.txt 13 14
    read outputs/c.txt
    write outputs/d.txt 11 12
    write outputs/d.txt 13 14
    read outputs/d.txt
    empty outputs/c.txt
    write outputs/d.txt 9 10
    read outputs/d.txt
    empty outputs/d.txt
    write outputs/e.txt 9 10
    read outputs/e.txt
    empty outputs/e.txt

We show that worker threads of a different file indeed execute concurrently. Running the command below:

    strace -ftCe trace=openat,close -p `pidof ./file_server` 2> debug.txt

we see that target files are opened (using `openat`) at the same time. Here are a few snippets of the output

Program correctness is proven in \ref pg_synchronization "*Synchronization*".
