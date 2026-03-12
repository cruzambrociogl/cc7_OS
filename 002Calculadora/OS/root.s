.global _start

_start:
    /* Set the stack pointer to 0x80200000 (2MB into RAM) */
    /* The stack grows downwards, so this leaves space for your code at 0x80000000 */
    ldr sp, =0x80200000

    /* Jump to the C main function */
    bl main

/* If main returns, just hang here */
hang:
    b hang