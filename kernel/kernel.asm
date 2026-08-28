BITS 64
ORG 0x100000

entry:
    mov rbx, [rdi+32]
    mov rcx, [rdi+24]
    mov r8, rbx
    imul r8, rcx
    mov r9, [rdi]
    mov rdi, r9
    mov ecx, r8d
    mov eax, 0x0000FF00
    rep stosd

.hang:
    cli
    hlt
    jmp .hang
