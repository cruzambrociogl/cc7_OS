#!/bin/bash
echo "Compiling..."
make clean && make

if [ $? -eq 0 ]; then
    echo ""
    echo "Running with 4 threads..."
    echo "========================="
    ./log_analyzer access.log 4
fi
