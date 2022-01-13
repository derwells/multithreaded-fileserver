\page pg_concurrency Concurrency

# Implementation
Each worker thread is associated with a target file. This allows for operations to a single file to be synchronized (see \ref pg_synchronization "Synchronization" for more info).

However, *worker threads associated with different files can run concurrently*. The blocked portion of each worker thread is guarded by an `in_lock` dependent on the target file. Thus, there is no mutual exclusivity between worker threads of different files. Commands associated with different files can run concurrently.

The only atomic operations between worker threads of different files are recording to `read.txt` and `empty.txt`. The program uses global locks to guard these files.

A complete picture of this can be found in the \ref pg_synchronization "Synchronization section".

# Proof
We can show that commands associated with different files can run concurrently using `gdb`. By examining thread states, we can ascertain that threads indeed run concurrently.

We do this by setting two breakpoints within the critical section. We do this by setting breakpoints in `.gdbinit`

\code{.unparsed}
    break 222
    break 296
    break 372
    run < conctest.txt
\endcode

From the top, these are for write, read, and empty respectively.

The input file `conctest.txt` will contains two lines of input. These will target two different files `a.txt` and `b.txt`.

Note that there are three types of commands. For completion, we show that every combination of two commands can run concurrently with each other.

For brevity, we look for test outputs that explicitly show the thread working within the `worker_*` function. We avoid outputs where either one of the threads is sleeping.

## write & write
For testing write command concurreny with another write, we use the `contest.txt` below
\code{.unparsed}
    write a.txt A
    write b.txt B
\endcode

Indeed, we see that the spawned threads are executing `worker_write()` at the same time.

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=1]{sync_overview.png}
	\caption{`write`, `write` concurrency}
	\label{overview}
\end{figure}
\endlatexonly

## write & read
\code{.unparsed}
    write a.txt A
    read b.txt
\endcode

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=1]{sync_overview.png}
	\caption{`write`, `read` concurrency}
	\label{overview}
\end{figure}
\endlatexonly
## write & empty
\code{.unparsed}
    write a.txt A
    empty b.txt
\endcode

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=1]{sync_overview.png}
	\caption{`write`, `empty` concurrency}
	\label{overview}
\end{figure}
\endlatexonly
## read & read
\code{.unparsed}
    read a.txt
    read b.txt
\endcode

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=1]{sync_overview.png}
	\caption{`read`, `read` concurrency}
	\label{overview}
\end{figure}
\endlatexonly
## read & empty
\code{.unparsed}
    read a.txt
    empty b.txt
\endcode

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=1]{sync_overview.png}
	\caption{`read`, `empty` concurrency}
	\label{overview}
\end{figure}
\endlatexonly
## empty & empty
\code{.unparsed}
    empty a.txt
    empty b.txt
\endcode

\latexonly
\begin{figure}[H]
    \centering
	\includegraphics[scale=1]{sync_overview.png}
	\caption{`empty`, `empty` concurrency}
	\label{overview}
\end{figure}
\endlatexonly

Program correctness is proven in \ref pg_synchronization "Synchronization".
