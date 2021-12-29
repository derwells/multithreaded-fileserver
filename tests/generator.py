import os
import subprocess
import string    
import random

READ_FILE = "read.txt"
EMPTY_FILE = "empty.txt"
CMD_FILE = "commands.txt"

TEST_IN = "tests/test.in"
WRITE_STR = "write {} {}"
READ_STR = "read {}"
EMPTY_STR = "empty {}"

READ, WRITE, EMPTY = 0, 1, 2

# INPUTS
N_FILES = 20
N_CMDS = 10


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
    path_len = 50 - len("outputs/.txt")
    read_seq = {}
    empty_seq = {}
    contents = {}

    files = [
        "outputs/{}.txt".format(
            n_rand_alnum(
                random.randrange(1, path_len + 1)
            )
        ) for i in range(N_FILES)
    ]

    to_clean = set()

    actual_cmds = []
    for i in range(N_CMDS):
        for path in files:
            _input = n_rand_chars(random.randrange(1, 50 + 1))
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

            actual_cmds += [cmd]

    input("Press [enter] once file_server has finished execution:")

    fault_flag = False

    print("[ANALYZING] read.txt")
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
    for path in files:
        if not(path in read_seq.keys()):
            continue
        for i in range(len(read_seq[path])):
            if read_seq[path][i] != actual_read[path][i][:-1]:
                print(f"[FAULT] Inconsistent read at index {i}")
                print(f"[FAULT] generator: {len(read_seq[path][i])}\n", read_seq[path][i])
                print(f"[FAULT] read.txt: {len(actual_read[path][i])}\n", actual_read[path][i])
                fault_flag = True

    print("[ANALYZING] empty.txt")
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
    for path in files:
        if not(path in empty_seq.keys()):
            continue
        for i in range(len(empty_seq[path])):
            if empty_seq[path][i] != actual_empty[path][i][:-1]:
                print(f"[FAULT] Inconsistent empty at index {i}")
                print(f"[FAULT] generator: {len(empty_seq[path][i])}\n", empty_seq[path][i])
                print(f"[FAULT] read.txt: {len(actual_empty[path][i])}\n", actual_empty[path][i])
                fault_flag = True
    
    print("[ANALYZING] commands.txt")
    cmd_record = []
    with open(CMD_FILE, "r+") as f:
        cmd_record = f.readlines()
    for i, r in enumerate(cmd_record):
        _cmd = r.split(" ", 5)[-1]
        _cmd = _cmd[:-1]
        if _cmd != actual_cmds[i]:
            print(f"[FAULT] Inconsistent cmd at index {i}")
            fault_flag = True

    if not fault_flag:
        print(f"[GOOD] All tests passed")

    input("Press [enter] for cleanup: ")

    for path in to_clean:
        os.remove(path)


def main():
    # Cleanup
    open(READ_FILE, "w").close()
    open(EMPTY_FILE, "w").close()
    open(CMD_FILE, "w").close()
    open(TEST_IN, "w").close()

    # Run
    test_synchronization()

main()
