python3 generator.py 5 20 n

read_test=$(diff <(sort -t" " -s -k2,2 read.txt) <(sort -t" " -s -k2,2 outputs/gen_read.txt))

if [ -z "$read_test" ];
then
    echo "[PASS] read.txt"
else
    echo "[FAIL] read.txt"
fi

empty_test=$(diff <(sort -t" " -s -k2,2 empty.txt) <(sort -t" " -s -k2,2 outputs/gen_empty.txt))

if [ -z "$empty_test" ];
then
    echo "[PASS] empty.txt"
else
    echo "[FAIL] empty.txt"
fi
