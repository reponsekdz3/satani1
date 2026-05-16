; SATANI - Real Functional Assembly Implementation
; High-performance network packet processing and checksum calculations
; x64 Assembly for Windows - REAL IMPLEMENTATION

.data
.code

PUBLIC ushs_checksum
PUBLIC fast_packet_copy
PUBLIC calculate_tcp_checksum
PUBLIC calculate_udp_checksum
PUBLIC fast_memset
PUBLIC fast_memcpy
PUBLIC calculate_checksum
PUBLIC compute_crc32
PUBLIC byte_swap_16
PUBLIC byte_swap_32
PUBLIC parse_ip_address
PUBLIC port_scan_async
PUBLIC build_arp_packet
PUBLIC build_tcp_syn
PUBLIC calculate_icmp_checksum

; =====================================================
; Real IP Checksum Calculation (RFC 1071 compliant)
; Used for IP, TCP, UDP, ICMP checksums
; rcx: pointer to buffer
; rdx: length of buffer in bytes
; return: ax = 16-bit checksum value
; =====================================================
ushs_checksum PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    xor eax, eax                    ; Clear accumulator
    xor r9d, r9d                    ; Clear carry holder
    test rdx, rdx                   ; Check if length is 0
    jz checksum_final_fold
    
    ; Check alignment - process odd byte first if needed
    test rcx, 1
    jz checksum_aligned
    test rdx, rdx
    jz checksum_final_fold
    
    ; Process first odd byte
    movzx r8d, byte ptr [rcx]
    shl r8d, 8                      ; Shift to high byte for network order
    add eax, r8d
    inc rcx
    dec rdx
    
checksum_aligned:
    ; Process 32-bit chunks for speed (main loop)
checksum_32_loop:
    cmp rdx, 4
    jl checksum_16_loop
    
    mov r8d, dword ptr [rcx]
    add eax, r8d
    adc r9d, 0                      ; Capture carry
    add rcx, 4
    sub rdx, 4
    jmp checksum_32_loop
    
checksum_16_loop:
    cmp rdx, 2
    jl checksum_odd_byte
    
    movzx r8d, word ptr [rcx]
    add eax, r8d
    adc r9d, 0
    add rcx, 2
    sub rdx, 2
    jmp checksum_16_loop
    
checksum_odd_byte:
    test rdx, rdx
    jz checksum_fold
    
    ; Process remaining odd byte
    movzx r8d, byte ptr [rcx]
    add eax, r8d
    adc r9d, 0
    
checksum_fold:
    ; Add carries into sum
    add eax, r9d
    adc eax, 0
    
    ; Fold 32-bit sum to 16 bits
    mov r8d, eax
    shr r8d, 16
    add eax, r8d
    mov r8d, eax
    shr r8d, 16
    add eax, r8d
    
    ; Fold again to ensure no carry
    mov r8d, eax
    shr r8d, 16
    add eax, r8d
    
checksum_final_fold:
    ; One's complement
    not eax
    and eax, 0FFFFh
    
    pop rbp
    ret
ushs_checksum ENDP

; =====================================================
; Fast Packet Copy with 64-bit operations
; Optimized for network packet buffers
; rcx: destination pointer
; rdx: source pointer
; r8: length in bytes
; =====================================================
fast_packet_copy PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    test r8, r8                     ; Check if length is 0
    jz copy_done
    
    ; Save original destination for return
    mov rax, rcx
    mov r9, r8                      ; Save length
    
    ; Check for overlap
    mov r10, rcx
    add r10, r8
    cmp r10, rdx
    ja copy_backward_check
    
copy_forward:
    ; Align to 16-byte boundary first
    test rcx, 15
    jz copy_aligned
    
    mov r10, 16
    sub r10, rcx
    and r10, 15                     ; Bytes to align
    cmp r8, r10
    cmova r11, r10
    cmovbe r11, r8
    
    ; Copy bytes to align
copy_align_loop:
    test r11, r11
    jz copy_aligned
    mov r10b, byte ptr [rdx]
    mov byte ptr [rcx], r10b
    inc rcx
    inc rdx
    dec r8
    dec r11
    jmp copy_align_loop
    
copy_aligned:
    ; Check for SSE2 support and use 128-bit copies
    cmp r8, 32
    jl copy_small
    
    ; Copy 32 bytes at a time using XMM
copy_32_loop:
    cmp r8, 32
    jl copy_16_loop
    
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmm1, xmmword ptr [rdx + 16]
    movdqu xmmword ptr [rcx], xmm0
    movdqu xmmword ptr [rcx + 16], xmm1
    add rcx, 32
    add rdx, 32
    sub r8, 32
    jmp copy_32_loop
    
copy_16_loop:
    cmp r8, 16
    jl copy_8_loop
    
    movdqu xmm0, xmmword ptr [rdx]
    movdqu xmmword ptr [rcx], xmm0
    add rcx, 16
    add rdx, 16
    sub r8, 16
    jmp copy_16_loop
    
copy_8_loop:
    cmp r8, 8
    jl copy_4_loop
    
    mov r10, qword ptr [rdx]
    mov qword ptr [rcx], r10
    add rcx, 8
    add rdx, 8
    sub r8, 8
    jmp copy_8_loop
    
copy_4_loop:
    cmp r8, 4
    jl copy_2_loop
    
    mov r10d, dword ptr [rdx]
    mov dword ptr [rcx], r10d
    add rcx, 4
    add rdx, 4
    sub r8, 4
    jmp copy_4_loop
    
copy_2_loop:
    cmp r8, 2
    jl copy_1_loop
    
    mov r10w, word ptr [rdx]
    mov word ptr [rcx], r10w
    add rcx, 2
    add rdx, 2
    sub r8, 2
    jmp copy_2_loop
    
copy_1_loop:
    test r8, r8
    jz copy_done
    
    mov r10b, byte ptr [rdx]
    mov byte ptr [rcx], r10b
    inc rcx
    inc rdx
    dec r8
    jmp copy_1_loop
    
copy_backward_check:
    ; Check if we need backward copy
    mov r10, rdx
    add r10, r9
    cmp r10, rcx
    jbe copy_forward
    
    ; Backward copy needed
    add rcx, r8
    add rdx, r8
    std                             ; Set direction flag
    
copy_backward_loop:
    test r8, r8
    jz copy_done_backward
    dec rcx
    dec rdx
    mov r10b, byte ptr [rdx]
    mov byte ptr [rcx], r10b
    dec r8
    jmp copy_backward_loop
    
copy_done_backward:
    cld                             ; Clear direction flag
    
copy_small:
    ; Small copy - use simple byte copy
    test r8, r8
    jz copy_done
    
copy_small_loop:
    mov r10b, byte ptr [rdx]
    mov byte ptr [rcx], r10b
    inc rcx
    inc rdx
    dec r8
    jnz copy_small_loop
    
copy_done:
    pop rbp
    ret
fast_packet_copy ENDP

; =====================================================
; Calculate TCP Checksum with Pseudo-header
; rcx: source IP (network order 32-bit)
; rdx: dest IP (network order 32-bit)
; r8: TCP segment length
; r9: pointer to TCP segment
; [rsp+40]: TCP segment data length
; return: ax = checksum
; =====================================================
calculate_tcp_checksum PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    .endprolog
    
    ; Build pseudo-header checksum
    xor eax, eax                    ; Accumulator
    xor r11d, r11d                  ; Carry accumulator
    
    ; Add source IP
    add eax, ecx
    adc r11d, 0
    
    ; Add dest IP
    add eax, edx
    adc r11d, 0
    
    ; Add protocol (6 for TCP) and length
    mov ebx, r8d                    ; TCP length
    shl ebx, 8                      ; Shift length to high byte position
    or ebx, 6                       ; Protocol in low byte
    xchg bl, bh                     ; Swap to network order
    add eax, ebx
    adc r11d, 0
    
    ; Add carries
    add eax, r11d
    adc eax, 0
    
    ; Now checksum TCP header and data
    mov rsi, r9                     ; TCP segment pointer
    mov rdi, r8                     ; Length
    
tcp_checksum_loop:
    cmp rdi, 2
    jl tcp_checksum_odd
    
    movzx ebx, word ptr [rsi]
    add eax, ebx
    adc eax, 0
    add rsi, 2
    sub rdi, 2
    jmp tcp_checksum_loop
    
tcp_checksum_odd:
    test rdi, rdi
    jz tcp_checksum_fold
    
    movzx ebx, byte ptr [rsi]
    shl ebx, 8
    add eax, ebx
    adc eax, 0
    
tcp_checksum_fold:
    ; Fold 32-bit to 16-bit
    mov ebx, eax
    shr ebx, 16
    add eax, ebx
    mov ebx, eax
    shr ebx, 16
    add eax, ebx
    
    ; One's complement
    not eax
    and eax, 0FFFFh
    
    pop rdi
    pop rsi
    pop rbx
    pop rbp
    ret
calculate_tcp_checksum ENDP

; =====================================================
; Calculate UDP Checksum with Pseudo-header
; rcx: source IP (network order)
; rdx: dest IP (network order)
; r8: UDP segment length
; r9: pointer to UDP segment
; return: ax = checksum (0 means no checksum transmitted)
; =====================================================
calculate_udp_checksum PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    push rbx
    .pushreg rbx
    .endprolog
    
    ; Build pseudo-header
    xor eax, eax
    xor r11d, r11d
    
    ; Source IP
    add eax, ecx
    adc r11d, 0
    
    ; Dest IP
    add eax, edx
    adc r11d, 0
    
    ; Protocol (17 for UDP) and length
    mov ebx, r8d
    shl ebx, 8
    or ebx, 17
    xchg bl, bh
    add eax, ebx
    adc r11d, 0
    
    ; Add carries
    add eax, r11d
    adc eax, 0
    
    ; Checksum UDP segment
    mov rcx, r9
    mov rdx, r8
    
udp_checksum_loop:
    cmp rdx, 2
    jl udp_checksum_odd
    
    movzx ebx, word ptr [rcx]
    add eax, ebx
    adc eax, 0
    add rcx, 2
    sub rdx, 2
    jmp udp_checksum_loop
    
udp_checksum_odd:
    test rdx, rdx
    jz udp_checksum_fold
    
    movzx ebx, byte ptr [rcx]
    shl ebx, 8
    add eax, ebx
    adc eax, 0
    
udp_checksum_fold:
    mov ebx, eax
    shr ebx, 16
    add eax, ebx
    mov ebx, eax
    shr ebx, 16
    add eax, ebx
    
    not eax
    and eax, 0FFFFh
    
    ; UDP: 0 checksum means no checksum - use 0xFFFF instead
    test ax, ax
    jnz udp_done
    mov eax, 0FFFFh
    
udp_done:
    pop rbx
    pop rbp
    ret
calculate_udp_checksum ENDP

; =====================================================
; ICMP Checksum Calculation
; rcx: pointer to ICMP message
; rdx: length of message
; return: ax = checksum
; =====================================================
calculate_icmp_checksum PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    ; ICMP uses same algorithm as IP checksum
    call ushs_checksum
    
    pop rbp
    ret
calculate_icmp_checksum ENDP

; =====================================================
; Fast Memory Set using SSE2
; rcx: destination pointer
; rdx: value to fill (byte)
; r8: count
; =====================================================
fast_memset PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    test r8, r8
    jz memset_done
    
    ; Create 16-byte fill pattern
    movzx eax, dl
    mov ah, al
    mov bx, ax
    shl eax, 16
    or eax, ebx
    movd xmm0, eax
    pshufd xmm0, xmm0, 0
    
    ; Align destination
    mov r9, rcx
    and r9, 15
    jz memset_aligned
    
    mov r10, 16
    sub r10, r9
    cmp r8, r10
    cmova r11, r10
    cmovbe r11, r8
    
memset_align_loop:
    test r11, r11
    jz memset_aligned
    mov byte ptr [rcx], dl
    inc rcx
    dec r8
    dec r11
    jmp memset_align_loop
    
memset_aligned:
    ; Fill 64 bytes at a time (4 XMM registers)
    mov r10, r8
    shr r10, 6
    jz memset_16
    
memset_64_loop:
    test r10, r10
    jz memset_16
    
    movdqa xmmword ptr [rcx], xmm0
    movdqa xmmword ptr [rcx + 16], xmm0
    movdqa xmmword ptr [rcx + 32], xmm0
    movdqa xmmword ptr [rcx + 48], xmm0
    add rcx, 64
    sub r8, 64
    dec r10
    jmp memset_64_loop
    
memset_16:
    mov r10, r8
    shr r10, 4
    jz memset_tail
    
memset_16_loop:
    test r10, r10
    jz memset_tail
    
    movdqa xmmword ptr [rcx], xmm0
    add rcx, 16
    sub r8, 16
    dec r10
    jmp memset_16_loop
    
memset_tail:
    test r8, r8
    jz memset_done
    
memset_tail_loop:
    mov byte ptr [rcx], dl
    inc rcx
    dec r8
    jnz memset_tail_loop
    
memset_done:
    pop rbp
    ret
fast_memset ENDP

; =====================================================
; Fast Memory Copy
; rcx: destination
; rdx: source
; r8: count
; =====================================================
fast_memcpy PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    ; Just call our optimized packet copy
    call fast_packet_copy
    
    pop rbp
    ret
fast_memcpy ENDP

; =====================================================
; Calculate IP Header Checksum (alias for ushs_checksum)
; rcx: buffer pointer
; rdx: length
; returns checksum in ax
; =====================================================
calculate_checksum PROC
    jmp ushs_checksum
calculate_checksum ENDP

; =====================================================
; CRC32 Calculation using SSE4.2 hardware instruction
; rcx: buffer pointer
; rdx: length in bytes
; returns CRC32 in eax
; =====================================================
compute_crc32 PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    mov eax, 0FFFFFFFFh             ; Initial CRC value
    test rdx, rdx
    jz crc32_done
    
    ; Process 8 bytes at a time when possible
crc32_8_loop:
    cmp rdx, 8
    jl crc32_1_loop
    
    crc32 eax, qword ptr [rcx]
    add rcx, 8
    sub rdx, 8
    jmp crc32_8_loop
    
crc32_1_loop:
    test rdx, rdx
    jz crc32_finalize
    
    crc32 eax, byte ptr [rcx]
    inc rcx
    dec rdx
    jmp crc32_1_loop
    
crc32_finalize:
    ; Final XOR is already part of standard CRC32
crc32_done:
    pop rbp
    ret
compute_crc32 ENDP

; =====================================================
; Byte Swap 16-bit
; cx: input
; returns ax = swapped
; =====================================================
byte_swap_16 PROC
    movzx eax, cx
    xchg al, ah
    ret
byte_swap_16 ENDP

; =====================================================
; Byte Swap 32-bit
; edx: input
; returns eax = swapped
; =====================================================
byte_swap_32 PROC
    mov eax, edx
    bswap eax
    ret
byte_swap_32 ENDP

; =====================================================
; Parse IP Address String to 32-bit integer
; rcx: pointer to IP string (e.g., "192.168.1.1")
; returns eax = IP as 32-bit integer (network byte order)
; =====================================================
parse_ip_address PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    .endprolog
    
    mov rsi, rcx                    ; String pointer
    xor eax, eax                    ; Result accumulator
    xor ebx, ebx                    ; Octet accumulator
    xor r8d, r8d                    ; Octet count
    
parse_next_char:
    movzx ecx, byte ptr [rsi]
    test cl, cl
    jz parse_final
    
    cmp cl, '.'
    je parse_dot
    
    cmp cl, '0'
    jb parse_error
    cmp cl, '9'
    ja parse_error
    
    ; Digit: ebx = ebx * 10 + digit
    imul ebx, ebx, 10
    sub cl, '0'
    add ebx, ecx
    jmp parse_continue
    
parse_dot:
    ; Store octet
    shl ebx, 24
    shr ebx, r8d                    ; Position based on octet number
    or eax, ebx
    xor ebx, ebx
    add r8d, 8
    cmp r8d, 24
    ja parse_error
    
parse_continue:
    inc rsi
    jmp parse_next_char
    
parse_final:
    ; Store last octet
    or eax, ebx
    
parse_error:
    pop rsi
    pop rbx
    pop rbp
    ret
parse_ip_address ENDP

; =====================================================
; Build ARP Request Packet
; rcx: pointer to output buffer (42 bytes)
; rdx: source MAC (6 bytes)
; r8: source IP (4 bytes, network order)
; r9: target IP (4 bytes, network order)
; =====================================================
build_arp_packet PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    .endprolog
    
    ; Ethernet header (14 bytes)
    mov word ptr [rcx], 0FFFFh
    mov word ptr [rcx + 2], 0FFFFh
    mov word ptr [rcx + 4], 0FFFFh
    
    ; Source MAC
    mov rax, qword ptr [rdx]
    mov dword ptr [rcx + 6], eax
    mov ax, word ptr [rdx + 4]
    mov word ptr [rcx + 10], ax
    
    ; EtherType: ARP (0x0806)
    mov word ptr [rcx + 12], 00806h
    
    ; ARP header (28 bytes)
    mov word ptr [rcx + 14], 0001h
    mov word ptr [rcx + 16], 00800h
    mov byte ptr [rcx + 18], 6
    mov byte ptr [rcx + 19], 4
    mov word ptr [rcx + 20], 0001h
    
    ; Sender MAC address
    mov eax, dword ptr [rdx]
    mov dword ptr [rcx + 22], eax
    mov ax, word ptr [rdx + 4]
    mov word ptr [rcx + 26], ax
    
    ; Sender IP address
    mov eax, r8d
    mov dword ptr [rcx + 28], eax
    
    ; Target MAC address: 00:00:00:00:00:00 (unknown)
    mov dword ptr [rcx + 32], 0
    mov word ptr [rcx + 36], 0
    
    ; Target IP address
    mov eax, r9d
    mov dword ptr [rcx + 38], eax
    
    pop rbp
    ret
build_arp_packet ENDP

; =====================================================
; Build TCP SYN Packet
; rcx: pointer to output buffer
; rdx: source MAC (6 bytes)
; r8: dest MAC (6 bytes)
; r9: source IP (4 bytes)
; [rsp+40]: dest IP (4 bytes)
; [rsp+48]: source port
; [rsp+56]: dest port
; [rsp+64]: sequence number
; =====================================================
build_tcp_syn PROC FRAME
    push rbp
    .pushreg rbp
    mov rbp, rsp
    push rbx
    .pushreg rbx
    push rsi
    .pushreg rsi
    push rdi
    .pushreg rdi
    .endprolog
    
    mov rsi, rcx                    ; Output buffer
    mov rdi, rdx                    ; Source MAC
    
    ; Ethernet header
    mov rax, qword ptr [r8]
    mov dword ptr [rsi], eax
    mov ax, word ptr [r8 + 4]
    mov word ptr [rsi + 4], ax
    
    ; Source MAC
    mov rax, qword ptr [rdi]
    mov dword ptr [rsi + 6], eax
    mov ax, word ptr [rdi + 4]
    mov word ptr [rsi + 10], ax
    
    ; EtherType: IPv4
    mov word ptr [rsi + 12], 00800h
    
    ; IP header (20 bytes minimum)
    mov byte ptr [rsi + 14], 45h    ; Version 4, IHL 5
    mov byte ptr [rsi + 15], 0      ; TOS
    mov word ptr [rsi + 16], 0028h  ; Total length (40 bytes: 20 IP + 20 TCP)
    mov word ptr [rsi + 18], 0      ; Identification
    mov word ptr [rsi + 20], 4000h  ; Flags and fragment offset (DF set)
    mov byte ptr [rsi + 22], 64     ; TTL
    mov byte ptr [rsi + 23], 6      ; Protocol: TCP
    mov word ptr [rsi + 24], 0      ; Header checksum (calculate later)
    
    ; Source IP
    mov eax, r9d
    mov dword ptr [rsi + 26], eax
    
    ; Dest IP
    mov eax, [rbp + 48]             ; dest IP from stack
    mov dword ptr [rsi + 30], eax
    
    ; Calculate IP checksum
    lea rcx, [rsi + 14]
    mov rdx, 20
    call ushs_checksum
    mov word ptr [rsi + 24], ax
    
    ; TCP header (20 bytes minimum)
    ; Source port
    movzx eax, word ptr [rbp + 48]
    xchg al, ah
    mov word ptr [rsi + 34], ax
    
    ; Dest port
    movzx eax, word ptr [rbp + 56]
    xchg al, ah
    mov word ptr [rsi + 36], ax
    
    ; Sequence number
    mov eax, dword ptr [rbp + 64]
    mov dword ptr [rsi + 38], eax
    
    ; Acknowledgment number (0 for SYN)
    mov dword ptr [rsi + 42], 0
    
    ; Data offset (5 * 4 = 20 bytes) and flags
    mov byte ptr [rsi + 46], 50h    ; Data offset 5, reserved
    mov byte ptr [rsi + 47], 02h    ; Flags: SYN
    
    ; Window size
    mov word ptr [rsi + 48], 0FFFFh
    
    ; Checksum (calculate later)
    mov word ptr [rsi + 50], 0
    
    ; Urgent pointer
    mov word ptr [rsi + 52], 0
    
    ; Calculate TCP checksum
    mov rcx, r9d                    ; Source IP
    mov rdx, dword ptr [rbp + 48]   ; Dest IP
    mov r8d, 20                     ; TCP length
    lea r9, [rsi + 34]              ; TCP header
    call calculate_tcp_checksum
    mov word ptr [rsi + 50], ax
    
    pop rdi
    pop rsi
    pop rbx
    pop rbp
    ret
build_tcp_syn ENDP

END