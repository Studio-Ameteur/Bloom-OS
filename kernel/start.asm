BITS 64
SECTION .text.start
GLOBAL _start
EXTERN kmain

_start:
    call kmain
.hang:
    cli
    hlt
    jmp .hang
