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

# Use all ASCII printable characters
def n_rand_chars(n):
    return "".join(
            [   
                chr(x) for x in
                choices(
                    list(range(32, 126 + 1)),
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

    read_seq = {}   # expected read.txt per file
    empty_seq = {}  # expected empty.txt per file
    contents = {}   # expect file contents per file

    # generate filenames
    files = [
        "{}.txt".format(
            n_rand_alnum(
                random.randrange(1, path_len + 1)
            )
        ) for i in range(N_FILES)
    ]

    # files that have a write command issued
    to_clean = set()

    ##################
    # Build test.in
    ##################
    actual_cmds = []
    for i in range(N_CMDS):
        # shuffle files - no input pattern
        random.shuffle(files)

        for path in files:

            # restrict to alphanumeric?
            if RESTRICT:
                _input = n_rand_alnum(random.randrange(1, 50 + 1))
            else:
                _input = n_rand_chars(random.randrange(1, 50 + 1))

            # choose command to issue
            choice = choices([READ, WRITE, EMPTY], k=1)[0]

            # make sure at least 1 write command occurs per file
            if (
                i == N_CMDS - 1 and
                (not (path in to_clean))
            ):
                choice = WRITE

            # if read command is issued
            if choice == READ:
                if not (path in read_seq.keys()):
                    read_seq[path] = [] # initialize if no read command so far

                # if file has been written to
                c = contents[path][-1] if path in contents.keys() else "FILE DNE"

                # append to expected read.txt
                read_seq[path].append(c)
                cmd = build_read(path) # add to test.in

            # if wrote command is issued
            elif choice == WRITE:
                to_clean.add(path)
                if not (path in contents.keys()):
                    # initialize if no write command so far
                    contents[path] = []
                    tmp = _input
                else:
                    # build new file contents
                    tmp = contents[path][-1]  + _input

                # update file contents
                contents[path].append(tmp)

                cmd = build_write(path, _input) # add to test.in

            # if read command is issued
            elif choice == EMPTY:
                if not (path in empty_seq.keys()):
                    empty_seq[path] = [] # initialize
                if path in contents.keys():
                    # empty out, save contents in c
                    c = contents[path][-1]
                    contents[path].append("")
                else:
                    # file does not exist
                    c = "FILE ALREADY EMPTY"

                empty_seq[path].append(c)
                cmd = build_empty(path) # add to test.in
            
            
            with open(TEST_IN, "a+") as f:
                f.write(cmd + "\n")

            actual_cmds += [cmd]

    ##################################
    # Generate expected output files +
    # Analyze commands.txt
    ##################################
    input("Press [enter] once file_server has finished execution:")

    fault_flag = False

    # read commands.txt
    cmd_record = []
    with open(CMD_FILE, "r+") as f:
        cmd_record = f.readlines()

    # cleanup target files
    for path in to_clean:
        try:
            os.remove(path)
        except:
            continue
    
    # dump expected read.txt into outputs/gen_read.txt 
    print("[GENERATING] read.txt")
    with open(GEN_READ, "w") as f:
        for path in files:
            if not (path in read_seq.keys()):
                continue
            for l in read_seq[path]:
                read_record = "read {}: {}\n".format(
                    path, l
                )
                f.write(read_record)

    # dump expected empty.txt into outputs/gen_empty.txt 
    print("[GENERATING] empty.txt")
    with open(GEN_EMPTY, "w") as f:
        for path in files:
            if not (path in empty_seq.keys()):
                continue
            for l in empty_seq[path]:
                empty_record = "empty {}: {}\n".format(
                    path, l
                )
                f.write(empty_record)

    # parse and compare commands.txt lines 
    # to sequence of commands issues
    print("[ANALYZING] commands.txt")
    for i, actual in enumerate(actual_cmds): # per test.in command
        _cmd = cmd_record[i].split(" ", 5)[-1]
        _cmd = _cmd[:-1]
        if actual != _cmd:
            fault_flag = True

    if fault_flag:
        print("[FAIL] commands.txt")
    else:
        print("[PASS] commands.txt")

def main():
    # empty-out logging constructs and test.in
    open(READ_FILE, "w").close()
    open(EMPTY_FILE, "w").close()
    open(CMD_FILE, "w").close()
    open(TEST_IN, "w").close()

    # run test
    test_synchronization()

# make outputs directory
if not os.path.exists("outputs"):
    os.makedirs("outputs")
main()
