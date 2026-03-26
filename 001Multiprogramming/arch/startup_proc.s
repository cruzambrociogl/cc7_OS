.section .text
.syntax unified
.code 32
.globl _start

.extern main

_start:
    @ Stack pointer is set by the OS via the PCB before jumping here.
    @ Just call main.
    bl main
    b .
