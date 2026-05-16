.data?
.code

PUBLIC ushs_checksum

; x64 implementation of a fast checksum routine
; rcx: pointer to buffer
; rdx: length of buffer in bytes
; return: eax = 16-bit checksum value
ushs_checksum PROC
    xor rax, rax            ; checksum accumulator
    test rdx, rdx
    jz checksum_done

checksum_loop:
    movzx r8d, byte ptr [rcx]
    add eax, r8d
    inc rcx
    dec rdx
    jnz checksum_loop

    ; Fold result to 16 bits
    mov ecx, eax
    shr ecx, 16
    add eax, ecx
    mov ecx, eax
    shr ecx, 16
    add eax, ecx
    xor eax, 0xFFFF

checksum_done:
    ret
ushs_checksum ENDP

END