# Multithreaded File Server

Naive concurrent file server implementation. Made for the completion of CS 140 AY 21-22.

# Get Started

Run the following bash command

```
$ gcc -o file_server src/file_server.c src/llist.c -pthread
$ ./file_server
```

The program accepts three (3) instructions
 - `write <path> <string>`
   - append `<string>` to file defined by `<path>`
 - `read <path>`
   - read contents of `<path>` to `read.txt`
 - `empty <path>`
   - read contents of `<path>` to `empty.txt` then erase contents

# Synchronization

As per project specs, each command has an associated worker thread. Worker threads are grouped by their target filepath `<path>`. *Worker threads in a group are organized using hand-over-hand locking. This ensures that older threads in a group **must** run first before newer threads can.*

*Worker threads in different groups can run concurrently.*

# Testing

Testing was done using `autotest.sh`. Set-up exec permissions using
```
chmod +x autotest.sh
```

and run it using

```
autotest.sh <N_FILES> <N_CMDS> <USE_ALPHANUMERIC_FILENAMES>
```

This writes a randomly-generated sequence of commands to `test.in`. Note that the number of generated commands is `N_FILES * N_CMDS`.

Run `./file_server` in a separate terminal then copy-paste `test.in`. Once the commands have finished executing, respond to the `autotest.sh` prompt

```
Press [enter] once file_server has finished execution:
```

## Example
The command below generates 100 commmands with complicated filenames.

```
autotest.sh 5 20 n
```
