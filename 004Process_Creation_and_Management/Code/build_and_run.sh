#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

TASK="$1"

if [ -z "$TASK" ]; then
    echo "Usage: ./build_and_run.sh <task>"
    echo "  ./build_and_run.sh task1_fork"
    echo "  ./build_and_run.sh task2_sync"
    echo "  ./build_and_run.sh task3_pipe"
    echo "  ./build_and_run.sh task4_multi"
    echo "  ./build_and_run.sh task5_shm"
    echo "  ./build_and_run.sh all       (build all)"
    exit 1
fi

rm -f bin/*.o

build_task() {
    local name="$1"
    if [ ! -f "${name}.c" ]; then
        echo "File ${name}.c not found"
        return 1
    fi
    echo "Compiling ${name}.c..."
    gcc -Wall -o "bin/${name}" "${name}.c"
    echo "Built bin/${name}"
}

if [ "$TASK" == "all" ]; then
    for src in task*.c; do
        name="${src%.c}"
        build_task "$name"
    done
    echo "All tasks built."
else
    build_task "$TASK"
    echo "Running bin/${TASK}..."
    echo "---"
    "bin/${TASK}"
fi
