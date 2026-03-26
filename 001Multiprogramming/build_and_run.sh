#!/bin/bash

# Build script for Multiprogramming Lab
# Produces 3 separate binaries: os.bin, p1.bin, p2.bin
#
# Usage:
#   ./build_and_run.sh              # Default: build for BeagleBone
#   ./build_and_run.sh beagle       # Build for BeagleBone
#   ./build_and_run.sh qemu         # Build + run in QEMU
#   ./build_and_run.sh qemu_build   # Build for QEMU only (no run)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

ARM=arm-none-eabi
TARGET="${1:-beagle}"

# ============================================================================
# Target configuration
# ============================================================================
if [ "$TARGET" = "qemu" ] || [ "$TARGET" = "qemu_build" ] || [ "$TARGET" = "qemu_build_only" ]; then
    CFLAGS="-mcpu=arm926ej-s -mfloat-abi=soft -ffreestanding -nostdlib -nostartfiles -g -Wall -O2"
    BSP_DIR="bsp/qemu"
    STARTUP="arch/startup_qemu.s"
    LD_OS="linker/qemu_os.ld"
    LD_P1="linker/qemu_p1.ld"
    LD_P2="linker/qemu_p2.ld"
    OUT="out/qemu"
    P1_ADDR="0x00100000"
    P2_ADDR="0x00200000"
    echo "=== Target: QEMU VersatilePB ==="
elif [ "$TARGET" = "beagle" ] || [ "$TARGET" = "hardware" ]; then
    CFLAGS="-mcpu=cortex-a8 -mfloat-abi=soft -ffreestanding -nostdlib -nostartfiles -g -Wall -O2"
    BSP_DIR="bsp/beagle"
    STARTUP="arch/startup_beagle.s"
    LD_OS="linker/os.ld"
    LD_P1="linker/p1.ld"
    LD_P2="linker/p2.ld"
    OUT="out/beagle"
    P1_ADDR="0x82100000"
    P2_ADDR="0x82200000"
    echo "=== Target: BeagleBone Black ==="
else
    echo "Usage: $0 [beagle|qemu|qemu_build|qemu_build_only]"
    exit 1
fi

# Include path for board.h
INCLUDES="-I $BSP_DIR"

# Clean previous build
echo ""
echo "Cleaning previous build..."
rm -rf "$OUT"
mkdir -p "$OUT"

# ============================================================================
# OS Binary
# ============================================================================
echo ""
echo "=== Building OS ==="

echo "  Assembling arch/boot.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/boot.o arch/boot.s

echo "  Assembling $STARTUP..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/startup.o $STARTUP

echo "  Compiling $BSP_DIR/uart_hw.c..."
$ARM-gcc $CFLAGS $INCLUDES -c $BSP_DIR/uart_hw.c  -o $OUT/uart_hw.o

echo "  Compiling $BSP_DIR/timer_hw.c..."
$ARM-gcc $CFLAGS $INCLUDES -c $BSP_DIR/timer_hw.c -o $OUT/timer_hw.o

echo "  Compiling hal/uart.c..."
$ARM-gcc $CFLAGS $INCLUDES -c hal/uart.c            -o $OUT/uart.o

echo "  Compiling hal/timer.c..."
$ARM-gcc $CFLAGS $INCLUDES -c hal/timer.c           -o $OUT/timer.o

echo "  Compiling lib/stdio.c..."
$ARM-gcc $CFLAGS $INCLUDES -c lib/stdio.c           -o $OUT/stdio.o

echo "  Compiling os/pcb.c..."
$ARM-gcc $CFLAGS $INCLUDES -c os/pcb.c              -o $OUT/pcb.o

echo "  Compiling os/main.c..."
$ARM-gcc $CFLAGS $INCLUDES -c os/main.c             -o $OUT/os_main.o

echo "  Linking os.elf..."
$ARM-gcc $CFLAGS -T $LD_OS \
    $OUT/startup.o $OUT/boot.o \
    $OUT/uart_hw.o $OUT/timer_hw.o \
    $OUT/uart.o $OUT/timer.o \
    $OUT/stdio.o $OUT/pcb.o $OUT/os_main.o \
    -o $OUT/os.elf

echo "  Converting to os.bin..."
$ARM-objcopy -O binary $OUT/os.elf $OUT/os.bin

# ============================================================================
# P1 Binary
# ============================================================================
echo ""
echo "=== Building P1 ==="

echo "  Assembling arch/startup_proc.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/startup_proc.o arch/startup_proc.s

echo "  Assembling arch/helpers.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/helpers.o arch/helpers.s

echo "  Compiling P1/main.c..."
$ARM-gcc $CFLAGS $INCLUDES -c P1/main.c             -o $OUT/p1_main.o

echo "  Linking p1.elf..."
$ARM-gcc $CFLAGS -T $LD_P1 \
    $OUT/startup_proc.o $OUT/helpers.o \
    $OUT/uart_hw.o $OUT/uart.o $OUT/stdio.o \
    $OUT/p1_main.o \
    -o $OUT/p1.elf

echo "  Converting to p1.bin..."
$ARM-objcopy -O binary $OUT/p1.elf $OUT/p1.bin

# ============================================================================
# P2 Binary
# ============================================================================
echo ""
echo "=== Building P2 ==="

echo "  Compiling P2/main.c..."
$ARM-gcc $CFLAGS $INCLUDES -c P2/main.c             -o $OUT/p2_main.o

echo "  Linking p2.elf..."
$ARM-gcc $CFLAGS -T $LD_P2 \
    $OUT/startup_proc.o $OUT/helpers.o \
    $OUT/uart_hw.o $OUT/uart.o $OUT/stdio.o \
    $OUT/p2_main.o \
    -o $OUT/p2.elf

echo "  Converting to p2.bin..."
$ARM-objcopy -O binary $OUT/p2.elf $OUT/p2.bin

# ============================================================================
# Done
# ============================================================================
echo ""
echo "========================================="
echo "Build complete!"
echo "  $OUT/os.bin"
echo "  $OUT/p1.bin"
echo "  $OUT/p2.bin"
echo "========================================="

if [ "$TARGET" = "beagle" ] || [ "$TARGET" = "hardware" ]; then
    echo ""
    echo "U-Boot commands:"
    echo "  loady 0x82000000    -> send os.bin"
    echo "  loady 0x82100000    -> send p1.bin"
    echo "  loady 0x82200000    -> send p2.bin"
    echo "  go 0x82000000"
fi

if [ "$TARGET" = "qemu" ]; then
    echo ""
    echo "Launching QEMU..."
    qemu-system-arm \
        -M versatilepb \
        -m 64M \
        -nographic \
        -kernel $OUT/os.elf \
        -device loader,file=$OUT/p1.bin,addr=$P1_ADDR \
        -device loader,file=$OUT/p2.bin,addr=$P2_ADDR
fi
