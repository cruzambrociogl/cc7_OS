#!/bin/bash

# Build script for Multiprogramming Lab
# Produces 3 separate binaries: os.bin, p1.bin, p2.bin

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

ARM=arm-none-eabi
CFLAGS="-mcpu=cortex-a8 -mfloat-abi=soft -ffreestanding -nostdlib -nostartfiles -g -Wall -O2"
OUT=out

# Clean previous build
echo "Cleaning previous build..."
rm -f "$OUT"/*.o "$OUT"/*.elf "$OUT"/*.bin
mkdir -p "$OUT"

# ============================================================================
# OS Binary — arch + hal + bsp + lib + os → os.bin @ 0x82000000
# ============================================================================
echo ""
echo "=== Building OS ==="

echo "  Assembling arch/boot.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/boot.o arch/boot.s

echo "  Assembling arch/startup_beagle.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/startup.o arch/startup_beagle.s

echo "  Compiling bsp/beagle/uart_hw.c..."
$ARM-gcc $CFLAGS -c bsp/beagle/uart_hw.c  -o $OUT/uart_hw.o

echo "  Compiling bsp/beagle/timer_hw.c..."
$ARM-gcc $CFLAGS -c bsp/beagle/timer_hw.c -o $OUT/timer_hw.o

echo "  Compiling hal/uart.c..."
$ARM-gcc $CFLAGS -c hal/uart.c            -o $OUT/uart.o

echo "  Compiling hal/timer.c..."
$ARM-gcc $CFLAGS -c hal/timer.c           -o $OUT/timer.o

echo "  Compiling lib/stdio.c..."
$ARM-gcc $CFLAGS -c lib/stdio.c           -o $OUT/stdio.o

echo "  Compiling os/pcb.c..."
$ARM-gcc $CFLAGS -c os/pcb.c              -o $OUT/pcb.o

echo "  Compiling os/main.c..."
$ARM-gcc $CFLAGS -c os/main.c             -o $OUT/os_main.o

echo "  Linking os.elf..."
$ARM-gcc $CFLAGS -T linker/os.ld \
    $OUT/startup.o $OUT/boot.o \
    $OUT/uart_hw.o $OUT/timer_hw.o \
    $OUT/uart.o $OUT/timer.o \
    $OUT/stdio.o $OUT/pcb.o $OUT/os_main.o \
    -o $OUT/os.elf

echo "  Converting to os.bin..."
$ARM-objcopy -O binary $OUT/os.elf $OUT/os.bin

# ============================================================================
# P1 Binary — startup_proc + uart + stdio + P1/main → p1.bin @ 0x82100000
# ============================================================================
echo ""
echo "=== Building P1 ==="

echo "  Assembling arch/startup_proc.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/startup_proc.o arch/startup_proc.s

echo "  Assembling arch/helpers.s..."
$ARM-gcc -c $CFLAGS -x assembler-with-cpp -o $OUT/helpers.o arch/helpers.s

echo "  Compiling P1/main.c..."
$ARM-gcc $CFLAGS -c P1/main.c             -o $OUT/p1_main.o

echo "  Linking p1.elf..."
$ARM-gcc $CFLAGS -T linker/p1.ld \
    $OUT/startup_proc.o $OUT/helpers.o \
    $OUT/uart_hw.o $OUT/uart.o $OUT/stdio.o \
    $OUT/p1_main.o \
    -o $OUT/p1.elf

echo "  Converting to p1.bin..."
$ARM-objcopy -O binary $OUT/p1.elf $OUT/p1.bin

# ============================================================================
# P2 Binary — startup_proc + uart + stdio + P2/main → p2.bin @ 0x82200000
# ============================================================================
echo ""
echo "=== Building P2 ==="

echo "  Compiling P2/main.c..."
$ARM-gcc $CFLAGS -c P2/main.c             -o $OUT/p2_main.o

echo "  Linking p2.elf..."
$ARM-gcc $CFLAGS -T linker/p2.ld \
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
echo "  $OUT/os.bin  (load at 0x82000000)"
echo "  $OUT/p1.bin  (load at 0x82100000)"
echo "  $OUT/p2.bin  (load at 0x82200000)"
echo ""
echo "U-Boot commands:"
echo "  loady 0x82000000    → send os.bin"
echo "  loady 0x82100000    → send p1.bin"
echo "  loady 0x82200000    → send p2.bin"
echo "  go 0x82000000"
echo "========================================="
