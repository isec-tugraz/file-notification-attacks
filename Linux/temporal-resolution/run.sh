#!/usr/bin/env bash
# Measures the delay between a file being read and the IN_ACCESS event
# reaching userspace.
set -euo pipefail
cd "$(dirname "$0")"

make

TESTFILE="$(pwd)/testing_file"
head -c 1024 /dev/urandom > "$TESTFILE"   # accessor reads 1000x, 1 byte each

./build/watcher "$TESTFILE" op-watch &
WPID=$!
sleep 0.5   # let the watcher register its watch before we start reading

./build/accessor "$TESTFILE" op-accs
wait "$WPID"

python3 stats.py

rm -f "$TESTFILE"
