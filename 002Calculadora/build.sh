#!/bin/bash

# Universal Build & Run Script for Bare-Metal ARM
# Usage: ./build.sh [versatile|beagle] [debug]

# Exit immediately if a command exits with a non-zero status
set -e

# 1. Configuration
TARGET="${1:-versatile}" # Default to versatile if not specified
DEBUG_MODE="$2"          # Optional second argument for debugging
OUTPUT_DIR="bin"
PROJECT_NAME="application"

mkdir -p "$OUTPUT_DIR"

# Remove previous compiled objects and binaries
echo "Cleaning..."
rm -f "$OUTPUT_DIR"/*.o "$OUTPUT_DIR"/*.elf "$OUTPUT_DIR"/*.bin

# 2. Target Settings
if [ "$TARGET" == "versatile" ]; then
    echo "Target: VersatilePB (ARM926EJ-S)"
    MACHINE="versatilepb"
    CFLAGS="-mcpu=arm926ej-s -g -Wall -ffreestanding"
    STARTUP="OS/root.s"
elif [ "$TARGET" == "beagle" ]; then
    echo "Target: BeagleBone Black (Cortex-A8)"
    MACHINE="beagle"
    CFLAGS="-mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -g -Wall -O2 -ffreestanding"
    STARTUP="OS/root.s"
else
    echo "Error: Unknown target '$TARGET'. Use 'versatile' or 'beagle'."
    exit 1
fi

# 3. Assemble Startup
if [ -f "$STARTUP" ]; then
    echo "Assembling $STARTUP..."
    arm-none-eabi-as -g -o "$OUTPUT_DIR/startup.o" "$STARTUP"
else
    echo "Error: Startup file $STARTUP not found."
    exit 1
fi

# 4. Compile C Sources
# Dynamically find .c files in current directory and subdirectories (depth 2)
# Excludes the output directory
SOURCES=$(find . -maxdepth 2 -name "*.c" -not -path "./$OUTPUT_DIR/*" -not -name ".*")
INCLUDES="-I. -IOS -Ilib -Iprogram"
OBJ_FILES="$OUTPUT_DIR/startup.o"

for src in $SOURCES; do
    filename=$(basename "$src" .c)
    echo "Compiling $src..."
    arm-none-eabi-gcc -c $CFLAGS $INCLUDES -o "$OUTPUT_DIR/${filename}.o" "$src"
    OBJ_FILES="$OBJ_FILES $OUTPUT_DIR/${filename}.o"
done

# 5. Link & Run
echo "Linking..."
arm-none-eabi-gcc $CFLAGS -T linker.ld -nostartfiles -o "$OUTPUT_DIR/$PROJECT_NAME.elf" $OBJ_FILES

echo "Generating Binary..."
arm-none-eabi-objcopy -O binary "$OUTPUT_DIR/$PROJECT_NAME.elf" "$OUTPUT_DIR/$PROJECT_NAME.bin"

if [ "$DEBUG_MODE" == "only_build" ]; then
    echo "Build successful. Skipping QEMU execution."
    exit 0
fi

echo "Running QEMU ($MACHINE)..."
echo "To exit QEMU: Press 'Ctrl-a' then 'x'"

if [ "$DEBUG_MODE" == "debug" ]; then
    echo "Debug mode enabled. QEMU is paused waiting for GDB..."
    echo "Run 'arm-none-eabi-gdb $OUTPUT_DIR/$PROJECT_NAME.elf' in another terminal."
    qemu-system-arm -M "$MACHINE" -nographic -kernel "$OUTPUT_DIR/$PROJECT_NAME.elf" -s -S
else
    qemu-system-arm -M "$MACHINE" -nographic -kernel "$OUTPUT_DIR/$PROJECT_NAME.elf"
fi
