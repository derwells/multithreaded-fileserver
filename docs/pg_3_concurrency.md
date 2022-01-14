\page pg_concurrency Concurrency

# Implementation
Each worker thread is associated with a target file. This allows for operations to a single file to be synchronized (see \ref pg_synchronization "Synchronization" for more info).

However, *worker threads associated with different files can run concurrently*. The blocked portion of each worker thread is guarded by an `in_lock` dependent on the target file. Thus, there is no mutual exclusivity between worker threads of different files. Commands associated with different files can run concurrently (see \ref pg_synchronization "Synchronization" for more info).

The only atomic operations between worker threads of different files are recording to `read.txt` and `empty.txt`. The program uses global locks to guard these files (see \ref pg_execution "Execution").

A complete picture of this can be found in the \ref pg_synchronization "Synchronization section".

# Concurrency Proof
\anchor level3_proof
We can show that commands associated with different files can run concurrently using `gdb`. By examining thread states, we can ascertain that threads indeed run concurrently. **When performing this test, add the `-g` flag to the compilation of `./file_server`**.

We do this by setting two breakpoints within the critical section. We do this by setting breakpoints in `.gdbinit`

\code{.unparsed}

break 210
break 222
break 259
break 296
break 332
break 372
run

\endcode

Each pair from the top are for write, read, and empty respectively.

After running, we input the commands below (last line is a newline). Once the breakpoints are reached, we run `info threads` in gdb. This shows that the threads are executing concurrently.

\code{.unparsed}

write outputs/a.txt 1
read outputs/b.txt 1
empty outputs/c.txt 1
read outputs/d.txt 1
write outputs/e.txt 1
empty outputs/f.txt 1
read outputs/g.txt 1

\endcode

@image latex conctest_1.png

Note that we can `continue` until another breakpoint is reached.

@image latex conctest_2.png

Indeed the threads are executing concurrently. Note that nanosleep is triggered by `r_simulate_access()`. This is within the critical section.

Program correctness is proven in \ref pg_synchronization "Synchronization".
