#!/bin/sh
set -e
cd "$(dirname "$0")/.."
for d in build*/; do
    echo "=== $d ==="
    cd "$d"
    ninja clean
    ninja
    ninja test
    cd ..
done
