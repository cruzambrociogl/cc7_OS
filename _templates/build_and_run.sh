#!/bin/bash

# Build and Run Script for Interrupt Handler Lab
# BeagleBone Black Bare-Metal Application

# Exit immediately if a command exits with a non-zero status
set -e

# 1. Configuration
# ./build_and_run.sh              -> Builds for BeagleBone (default), produces bin/program.bin
# ./build_and_run.sh hardware     -> Same as above
# ./build_and_run.sh qemu         -> Builds for QEMU + runs it
# ./build_and_run.sh qemu_build_only -> Builds for QEMU only (used by VS Code debug)

TARGET="${1:-hardware}"

# Run from script directory so paths work from anywhere
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f bin/*.o bin/program.elf bin/program.bin
mkdir -p bin

# 2. Target Settings
if [ "$TARGET" == "qemu" ] || [ "$TARGET" == "qemu_build_only" ]; then
    echo "Target: VersatilePB (QEMU)"
    MACHINE="versatilepb"
    CFLAGS="-mcpu=arm926ej-s -g -Wall -O2 -nostdlib -nostartfiles -ffreestanding -DQEMU"
    LINKER_SCRIPT="linker_versatile.ld"
elif [ "$TARGET" == "hardware" ]; then
    echo "Target: BeagleBone Black (Hardware)"
    CFLAGS="-mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -g -Wall -O2 -nostdlib -nostartfiles -ffreestanding"
    LINKER_SCRIPT="linker.ld"
fi

echo "Assembling OS/root.s..."
# Use gcc to assemble so we can use preprocessor macros (#ifdef)
arm-none-eabi-gcc -c $CFLAGS -x assembler-with-cpp -o bin/root.o OS/root.s

echo "Compiling OS/os.c..."
arm-none-eabi-gcc -c $CFLAGS \
    -o bin/os.o OS/os.c

echo "Linking object files..."
arm-none-eabi-gcc -nostartfiles -T $LINKER_SCRIPT $CFLAGS \
    -o bin/program.elf \
    bin/root.o bin/os.o

echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary bin/program.elf bin/program.bin

if [ "$TARGET" == "qemu" ]; then
    echo ""
    echo "Running QEMU ($MACHINE)..."
    echo "To exit QEMU: Press 'Ctrl-a' then 'x'"
    qemu-system-arm -M $MACHINE -nographic -kernel bin/program.elf
elif [ "$TARGET" == "qemu_build_only" ]; then
    echo ""
    echo "Build complete. Run with: ./build_and_run.sh qemu"
fi
