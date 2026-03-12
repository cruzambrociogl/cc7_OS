#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Remove previous compiled objects and binaries
echo "Cleaning up previous build files..."
rm -f root.o main.o calculadora.elf calculadora.bin

echo "Assembling startup.s..."
arm-none-eabi-as -o root.o root.s

echo "Compiling main.c..."
arm-none-eabi-gcc -c -o main.o main.c

echo "Linking object files..."
arm-none-eabi-ld -T linker.ld -o calculadora.elf root.o main.o

echo "Converting ELF to binary..."
arm-none-eabi-objcopy -O binary calculadora.elf calculadora.bin

echo "Running QEMU..."
qemu-system-arm -M versatilepb -nographic -kernel calculadora.elf
