import os
import subprocess
import string    
import random

READ_FILE = "read.txt"
EMPTY_FILE = "empty.txt"

TEST_IN = "tests/test.in"
WRITE_STR = "write {} {}"
READ_STR = "read {}"
EMPTY_STR = "empty {}"

READ, WRITE, EMPTY = 0, 1, 2

def n_rand_chars(n):
    return "".join(
            [   
                chr(x) for x in
                random.choices(
                    list(range(32, 127 + 1)),
                    k=n
                )
            ]
        )

def n_rand_alnum(n):
    return "".join(
            random.choices(
                string.ascii_uppercase + string.digits + string.ascii_lowercase,
                k=n
            )
        )

def build_write(path, input_):
    return WRITE_STR.format(
                path,
                input_
            )

def build_read(path):
    return READ_STR.format(path)

def build_empty(path):
    return EMPTY_STR.format(path)

def test_synchronization():
    n_files = 20
    n_cmds = 100
    path_len = 50 - len("outputs/.txt")
    read_seq = {}
    empty_seq = {}
    contents = {}

    files = [
        "outputs/{}.txt".format(n_rand_alnum(path_len)) for i in range(n_files)
    ]

    to_clean = set()

    for path in files:
        print(path)
    for i in range(n_cmds):
        for path in files:
            _input = n_rand_chars(50)
            choice = random.choices([READ, WRITE, EMPTY], k=1)[0]

            if choice == READ:
                if not (path in read_seq.keys()):
                    read_seq[path] = []
                c = contents[path][-1] if path in contents.keys() else "FILE DNE"
                read_seq[path].append(c)
                cmd = build_read(path)

            elif choice == WRITE:
                to_clean.add(path)
                if not (path in contents.keys()):
                    contents[path] = []
                    tmp = _input
                else:
                    tmp = contents[path][-1]  + _input

                contents[path].append(tmp)
                cmd = build_write(path, _input)

            elif choice == EMPTY:
                if not (path in empty_seq.keys()):
                    empty_seq[path] = []
                if path in contents.keys():
                    c = contents[path][-1]
                    contents[path].append("")
                else:
                    c = "FILE ALREADY EMPTY"

                empty_seq[path].append(c)
                cmd = build_empty(path)
            
            
            with open(TEST_IN, "a+") as f:
                f.write(cmd + "\n")

    input("Press [enter] when file_server has been run: ")

    print("***********READ")
    actual_read = {}
    read_record = []
    with open(READ_FILE, "r+") as f:
        read_record = f.readlines()
    for r in read_record:
        _, path, input_ = r.split(" ", 2)
        path = path[:-1]
        if not path in actual_read.keys():
            actual_read[path] = []
        actual_read[path].append(input_)
    print(read_seq.keys() <= actual_read.keys())
    for path in files:
        if not(path in read_seq.keys()):
            continue
        print(path)
        print(len(read_seq[path]), len(actual_read[path]))
        for i in range(len(read_seq[path])):
            if read_seq[path][i] + "\n" != actual_read[path][i]:
                print(f"READ: {len(read_seq[path][i])}\n", read_seq[path][i])
                print(f"ACTUAL: {len(actual_read[path][i])}\n", actual_read[path][i])

    print("***********EMPTY")
    actual_empty = {}
    empty_record = []
    with open(EMPTY_FILE, "r+") as f:
        empty_record = f.readlines()
    for r in empty_record:
        _, path, input_ = r.split(" ", 2)
        path = path[:-1]
        if not path in actual_empty.keys():
            actual_empty[path] = []
        actual_empty[path].append(input_)
    print(empty_seq.keys() >= actual_read.keys())
    for path in files:
        if not(path in empty_seq.keys()):
            continue
        print(path)
        print(len(empty_seq[path]), len(actual_empty[path]))
        for i in range(len(empty_seq[path])):
            if empty_seq[path][i] + "\n" != actual_empty[path][i]:
                print(f"READ: {len(empty_seq[path][i])}\n", empty_seq[path][i])
                print(f"ACTUAL: {len(actual_empty[path][i])}\n", actual_empty[path][i])


    input("Press [enter] for cleanup: ")

    for path in to_clean:
        os.remove(path)


def main():
    input("START: ")
    input("RUN: ")
    input("CLEANUP: ")


open(READ_FILE, "w").close()
open(EMPTY_FILE, "w").close()
open(TEST_IN, "w").close()
test_synchronization()