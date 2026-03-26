ENTRY(reset_handler)

MEMORY {
    RAM (rwx) : ORIGIN = 0x82000000, LENGTH = 64K
}

SECTIONS {
    .text : { *(.text*) } > RAM
    .rodata : { *(.rodata*) } > RAM
    .data : { *(.data*) } > RAM
    .bss : {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > RAM
}
