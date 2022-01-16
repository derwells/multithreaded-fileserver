\mainpage Introduction

This project is made for the completion of CS 140 21.1 Project 2. It is a **Level 4** implementation.

The implementation is divided into two files:
 - `defs.h`: contains `typedef struct` definitions and other useful macros.
 - `file_server.c`: driver program.
Two additional files are included. They were used in testing
- `generator.py`: Python script used for semi-automated testing
- `.gdbinit-sample`: GDB commands for testing Level 3
- `integrity.txt`: Small integrity test input

Refer to the program documentation for a detailed description of each file.

In-depth explanations for each level requirement can be found at
- \subpage pg_nonblocking "Non-blocking master" for Level 1 requirements.
- \subpage pg_execution "Command Execution" for Level 2 requirements.
- \subpage pg_concurrency "Concurrency" for Level 3 requirements.
- \subpage pg_synchronization "Synchronization" for Level 3, 4 requirements.

# Level Declaration
The following Levels were implemented
- Level 1
- Level 2
- Level 3
- Level 4

# Terminologies
- *Target file*: `<path/to/file>` of each command
- *Hand-over-hand locking*: locking mechanism for ordering threads; similar to the concept used in concurrent linked lists
- *Metadata*: Refers to information about a particular file; used for synchronization
- *Action*: Refers to read, write, or empty
