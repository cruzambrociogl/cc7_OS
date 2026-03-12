ENTRY(_start)

SECTIONS
{
    /* Set the load address to 0x80000000 (Start of DDR3 RAM on BeagleBone) */
    . = 0x80000000;

    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }

    /* Align to 8 bytes */
    . = ALIGN(8);
}