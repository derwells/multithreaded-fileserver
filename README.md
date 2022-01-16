# Introduction
This is a *Level 4* file server made for the completion of CS 140 Project 2.

Made by: WELLS, William Frederic SN 201803155

# Implementation

The implementation contains two files
- `file_server.c`: the file server program
- `defs.h`: headers and structs used by the file server

Compile the code using
```
gcc -o file_server file_server.c -pthread
```

and run `./file_server`. Logging files are generated relative to the current directory.

Debugging can be enabled by setting

```
#define DEBUGGING   1
```

in `defs.h`.

## Important Notes
- Include a trailing newline `\n` when copy-pasting inputs
- Only valid inputs are accepted
- All logging files are assumed to be empty



# Testing
Logging files can be standardized by running the command below
```
sort -t" " -s -k2,2 <read or empty>.txt
```
This groups and sorts each line by their target file while retaining the order they were written within the grouping.

Note that `./file_server` will run until manually terminated. To check whether all the worker threads have completed, run

```
htop -p `pidof ./file_server`
```

and wait until there are only *two threads left*. These are the `main` and `master` threads.

## Automated Testing
Below are the files used in automated testing
- `generator.py`: script for generating sample inputs `test.in` and expected outputs `outputs/gen_read.txt` and `outputs/gen_write.txt`
   - Run using `python3 generator.py <N_FILES> <N_CMDS> <ALNUM?>`
   - `N_FILES`: number of target files
   - `N_CMDS`: number of commands to issue per file
   - `ALNUM?`: `y` or `n`; restrict write inputs to alphanumeric characters?
   - Analyzes commands.txt to see if it is consistent with `test.in`
- `autotesh.sh`: script for running `generator.py` and comparing `read.txt` and `empty.txt` to `outputs/gen_read.txt` and `outputs/gen_write.txt` respectively
    - Done using `diff` on the standardized outputs

Setup and run the scripts using the commands below

```
$ chmod +x autotest.sh
$ ./autotest.sh (depends on OS)
```

On a different terminal, run the `./file_server` in the same folder.

Copy-paste the `test.in` file into the `./file_server` terminal. When the file server has finished executing all the test input commands, respond to the `autotest.sh` prompt. This will generate `outputs/gen_read.txt` and `outputs/gen_write.txt` as well as compare `commands.txt` to `test.in`. 

## Concurrency Test
Testing concurrency was done using GDB and `.gdbinit-sample`. The latter sets breakpoints within the critical sections of the worker threads. The threads are inspected using `info threads`. This should show that several threads are executing at once.

Make sure that `~/.gdbinit` has the follow content

```
set auto-load safe-path /
```

The concurrency test steps are show below

```
$ cp .gdbinit-sample .gdbinit
$ gcc -o file_server file_server.c -pthread -g
$ gdb file_server
```

Once in GDB, copy paste the sample input in `concurrency.test` and wait until the thread arrive at breakpoints. Once paused, run

```
(gdb) info threads
```

to view the thread states. To continue to the next breakpoint, run

```
(gdb) continue
```

## Integrity Test

The integrity test uses a predictable input in `integrity.test`.

Below is expected *standardized output* for `read.txt`

```
read a.txt: the
read a.txt: thequick
read a.txt: thequickbrown
read a.txt: lazydog
read b.txt: the
read b.txt: thequick
read b.txt: thequickbrown
read b.txt: lazydog

```

Below expected *standardized output* for `empty.txt`

```
empty a.txt: thequickbrownfox
empty a.txt: lazydog
empty b.txt: thequickbrownfox
empty b.txt: lazydog

```
