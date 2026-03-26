ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x82200000, LENGTH = 64K
}

SECTIONS {
    .text : { *(.text*) } > RAM
    .rodata : { *(.rodata*) } > RAM
    .data : { *(.data*) } > RAM
    .bss : { *(.bss*) *(COMMON) } > RAM
}
