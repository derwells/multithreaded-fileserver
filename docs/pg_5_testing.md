\page pg_testing Testing

Various testing procedures were created to supplement the explanations and proofs of the previous chapters.

# Testing Level 2, 4
A Python script was created to test execution and synchronization correctness. The entire program can be found in `generator.py` (see Appendix).

The program involves four (4) stages
 -# Cleanup old files
 -# Test input generation
    - Writing to `test.in`
 -# File server execution
    - Run `./file_server < path/to/test.in`
 -# Analysis of `read.txt`, `empty.txt`, `commands.txt`

The inputs and paths of the generated commands consist of a variable number of 
characters - including non-alphanumeric chars. They can have lengths of 1 to at 
most 50 characters.

Synchronization is tested by keeping track of the expected file contents
over the course of program execution. Every time a `read` or `empty` command
is generated, we track the expected output. *This maintains that commands
to the same file path must be ordered.*

Each entry of `read.txt` and `empty.txt` is parsed and grouped
by their file path. They are then checked against the expected output.

## Test sample
Attached is a [sample generated `test.in`](@ref sampletestin) accessing 20 files with 10 commands each.
Included are the `./file_server` outputs for [`read.txt`](@ref sampleread), [`empty.txt`](@ref sampleempty), 
and [`commands.txt`](@ref samplecommands).

The `./file_server` passess the sample input

   Press [enter] once file_server has finished execution:
   [ANALYZING] read.txt
   [ANALYZING] empty.txt
   [ANALYZING] commands.txt
   [GOOD] All tests passed
   Press [enter] for cleanup:


