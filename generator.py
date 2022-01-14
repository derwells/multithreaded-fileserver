import os
import subprocess
import string    
import random
import sys

from itertools import accumulate as _accumulate, repeat as _repeat
from bisect import bisect as _bisect

READ_FILE = "read.txt"
EMPTY_FILE = "empty.txt"
CMD_FILE = "commands.txt"

GEN_READ = "outputs/gen_read.txt"
GEN_EMPTY = "outputs/gen_empty.txt"

TEST_IN = "test.in"
WRITE_STR = "write {} {}"
READ_STR = "read {}"
EMPTY_STR = "empty {}"

READ, WRITE, EMPTY = 0, 1, 2

# INPUTS
N_FILES = int(sys.argv[1])
N_CMDS = int(sys.argv[2])
RESTRICT = True if sys.argv[3] == 'y' else False

# For Python 3.4
# Acquired from https://stackoverflow.com/questions/58915023
# Useful for executing within Linux 2.6.4 kernel
def choices(population, weights=None, *, cum_weights=None, k=1):
    """Return a k sized list of population elements chosen with replacement.
    If the relative weights or cumulative weights are not specified,
    the selections are made with equal probability.
    """
    n = len(population)
    if cum_weights is None:
        if weights is None:
            _int = int
            n += 0.0    # convert to float for a small speed improvement
            return [population[_int(random.random() * n)] for i in _repeat(None, k)]
        cum_weights = list(_accumulate(weights))
    elif weights is not None:
        raise TypeError('Cannot specify both weights and cumulative weights')
    if len(cum_weights) != n:
        raise ValueError('The number of weights does not match the population')
    bisect = _bisect
    total = cum_weights[-1] + 0.0   # convert to float
    hi = n - 1
    return [population[bisect(cum_weights, random.random() * total, 0, hi)]
            for i in _repeat(None, k)]


def n_rand_chars(n):
    return "".join(
            [   
                chr(x) for x in
                choices(
                    list(range(32, 127 + 1)),
                    k=n
                )
            ]
        )

def n_rand_alnum(n):
    return "".join(
            choices(
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
    path_len = 50 - len(".txt")
    read_seq = {}
    empty_seq = {}
    contents = {}

    files = [
        "{}.txt".format(
            n_rand_alnum(
                random.randrange(1, path_len + 1)
            )
        ) for i in range(N_FILES)
    ]

    to_clean = set()

    actual_cmds = []
    for i in range(N_CMDS):
        random.shuffle(files)
        for path in files:
            if RESTRICT:
                _input = n_rand_alnum(random.randrange(1, 50 + 1))
            else:
                _input = n_rand_chars(random.randrange(1, 50 + 1))
            choice = choices([READ, WRITE, EMPTY], k=1)[0]
            if (
                i == N_CMDS - 1 and
                (not (path in to_clean))
            ):
                choice = WRITE

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

    actual_read = {}
    read_record = []
    with open(READ_FILE, "r+") as f:
        read_record = f.readlines()

    actual_empty = {}
    empty_record = []
    with open(EMPTY_FILE, "r+") as f:
        empty_record = f.readlines()
    cmd_record = []
    with open(CMD_FILE, "r+") as f:
        cmd_record = f.readlines()

    # Cleanup
    for path in to_clean:
        os.remove(path)

    print("[ANALYZING] read.txt")
    for r in read_record:
        _, path, input_ = r.split(" ", 2)
        path = path[:-1]
        if not path in actual_read.keys():
            actual_read[path] = []
        actual_read[path].append(input_)
    for path in files:
        if not(path in read_seq.keys()):
            continue
        print(path)
        print(len(read_seq[path]), len(actual_read[path]))
        for i in range(len(read_seq[path])):
            if read_seq[path][i] != actual_read[path][i][:-1]:
                print("[FAULT] Inconsistent read at index {}".format(i))
                print("[FAULT] generator: {}\n".format(len(read_seq[path][i])), read_seq[path][i])
                print("[FAULT] read.txt: {}\n".format(len(actual_read[path][i])), actual_read[path][i])
                fault_flag = True

    print("[ANALYZING] empty.txt")
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
                print("[FAULT] Inconsistent empty at index {}".format(i))
                print("[FAULT] generator: {}\n".format(len(empty_seq[path][i])), empty_seq[path][i])
                print("[FAULT] read.txt: {}\n".format(len(actual_empty[path][i])), actual_empty[path][i])
                fault_flag = True
    
    print("[ANALYZING] commands.txt")
    for i, r in enumerate(cmd_record):
        _cmd = r.split(" ", 5)[-1]
        _cmd = _cmd[:-1]
        if _cmd != actual_cmds[i]:
            fault_flag = True

    if not fault_flag:
        print("[GOOD] All tests passed")



def main():
    # Cleanup
    open(READ_FILE, "w").close()
    open(EMPTY_FILE, "w").close()
    open(CMD_FILE, "w").close()
    open(TEST_IN, "w").close()

    # Run
    test_synchronization()

main()
