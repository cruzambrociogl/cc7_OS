ENTRY(_start)

SECTIONS
{
    . = 0x10000;

    .text : { *(.text) }
    .data : { *(.data) }
    .bss : { *(.bss) }

    . = ALIGN(8);
    _stack_bottom = .;
    . = . + 0x4000; /* 16KB Stack */
    _stack_top = .;
}