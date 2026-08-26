BITS 64
ORG 0x0

start:
    cli
.hang:
    hlt
    jmp .hang
