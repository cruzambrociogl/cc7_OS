ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x82100000, LENGTH = 64K
}

SECTIONS {
    .text : { *(.text*) } > RAM
    .rodata : { *(.rodata*) } > RAM
    .data : { *(.data*) } > RAM
    .bss : { *(.bss*) *(COMMON) } > RAM
}
