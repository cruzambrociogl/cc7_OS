#!/bin/bash

# Build and Run Script for Interrupt Handler Lab
# Supports: QEMU (VersatilePB) and BeagleBone Black (Hardware)

set -e

# 1. Configuration
# ./build_and_run.sh              -> Builds for BeagleBone (default), produces out/beagle/program.bin
# ./build_and_run.sh hardware     -> Same as above
# ./build_and_run.sh qemu         -> Builds for QEMU + runs it
# ./build_and_run.sh qemu_build_only -> Builds for QEMU only (used by VS Code debug)

TARGET="${1:-hardware}"

# Run from script directory so paths work from anywhere
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# 2. Target Settings
if [ "$TARGET" == "qemu" ] || [ "$TARGET" == "qemu_build_only" ]; then
    echo "Target: VersatilePB (QEMU)"
    MACHINE="versatilepb"
    CFLAGS="-mcpu=arm926ej-s -g -Wall -O2 -nostdlib -nostartfiles -ffreestanding"
    LINKER_SCRIPT="linker/qemu.ld"
    ARCH_SRC="arch/boot.s arch/startup_qemu.s"
    BSP_SRC="bsp/qemu/uart_hw.c bsp/qemu/timer_hw.c"
    OUT_DIR="out/qemu"
elif [ "$TARGET" == "hardware" ]; then
    echo "Target: BeagleBone Black (Hardware)"
    CFLAGS="-mcpu=cortex-a8 -mfpu=neon -mfloat-abi=hard -g -Wall -O2 -nostdlib -nostartfiles -ffreestanding"
    LINKER_SCRIPT="linker/beagle.ld"
    ARCH_SRC="arch/boot.s arch/startup_beagle.s"
    BSP_SRC="bsp/beagle/uart_hw.c bsp/beagle/timer_hw.c"
    OUT_DIR="out/beagle"
fi

# Common sources (always compiled)
HAL_SRC="hal/uart.c hal/timer.c"
LIB_SRC="lib/stdio.c"
APP_SRC="app/os.c"

# Clean and prepare output directory
echo "Cleaning up previous build files..."
rm -f "$OUT_DIR"/*.o "$OUT_DIR"/program.elf "$OUT_DIR"/program.bin
mkdir -p "$OUT_DIR"

# 3. Compile arch (assembly)
for src in $ARCH_SRC; do
    name=$(basename "$src" .s)
    echo "Assembling $src..."
    arm-none-eabi-gcc -c $CFLAGS -x assembler-with-cpp -o "$OUT_DIR/${name}.o" "$src"
done

# 4. Compile BSP (platform-specific C)
for src in $BSP_SRC; do
    name=$(basename "$src" .c)
    echo "Compiling $src..."
    arm-none-eabi-gcc -c $CFLAGS -o "$OUT_DIR/${name}.o" "$src"
done

# 5. Compile HAL (generic C)
for src in $HAL_SRC; do
    name=$(basename "$src" .c)
    echo "Compiling $src..."
    arm-none-eabi-gcc -c $CFLAGS -o "$OUT_DIR/${name}.o" "$src"
done

# 6. Compile lib
for src in $LIB_SRC; do
    name=$(basename "$src" .c)
    echo "Compiling $src..."
    arm-none-eabi-gcc -c $CFLAGS -o "$OUT_DIR/${name}.o" "$src"
done

# 7. Compile app
for src in $APP_SRC; do
    name=$(basename "$src" .c)
    echo "Compiling $src..."
    arm-none-eabi-gcc -c $CFLAGS -o "$OUT_DIR/${name}.o" "$src"
done

# 8. Link
echo "Linking object files..."
arm-none-eabi-gcc -nostartfiles -T "$LINKER_SCRIPT" $CFLAGS \
    -o "$OUT_DIR/program.elf" \
    "$OUT_DIR"/*.o

# 9. Convert ELF to binary
echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary "$OUT_DIR/program.elf" "$OUT_DIR/program.bin"

# 10. Run (if QEMU)
if [ "$TARGET" == "qemu" ]; then
    echo ""
    echo "Running QEMU ($MACHINE)..."
    echo "To exit QEMU: Press 'Ctrl-a' then 'x'"
    qemu-system-arm -M $MACHINE -nographic -kernel "$OUT_DIR/program.elf"
elif [ "$TARGET" == "qemu_build_only" ]; then
    echo ""
    echo "Build complete. Run with: ./build_and_run.sh qemu"
fi
