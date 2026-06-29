global isr_table
extern exception_handler

%macro ISR_NOERR 1
isr_%1:
    push 0          ; fake error code
    push %1         ; vector number
    jmp isr_common
%endmacro

%macro ISR_ERR 1
isr_%1:
    push %1         ; vector (error code already on stack)
    jmp isr_common
%endmacro

; CPU exceptions: 0-7 no error, 8 has error, 9 no, 10-14 have error, 15 no, 16 no, 17 has error, 18-20 no
ISR_NOERR 0   ; #DE divide by zero
ISR_NOERR 1   ; #DB debug
ISR_NOERR 2   ; NMI
ISR_NOERR 3   ; #BP breakpoint
ISR_NOERR 4   ; #OF overflow
ISR_NOERR 5   ; #BR bound range
ISR_NOERR 6   ; #UD invalid opcode
ISR_NOERR 7   ; #NM device not available
ISR_ERR   8   ; #DF double fault
ISR_NOERR 9   ; coprocessor overrun (legacy)
ISR_ERR   10  ; #TS invalid TSS
ISR_ERR   11  ; #NP segment not present
ISR_ERR   12  ; #SS stack fault
ISR_ERR   13  ; #GP general protection
ISR_ERR   14  ; #PF page fault
ISR_NOERR 15
ISR_NOERR 16  ; #MF x87 FPU
ISR_ERR   17  ; #AC alignment check
ISR_NOERR 18  ; #MC machine check
ISR_NOERR 19  ; #XM SIMD FPU
ISR_NOERR 20  ; #VE virtualization

; Fill the rest as generic stubs
%assign i 21
%rep 235
ISR_NOERR i
%assign i i+1
%endrep

isr_common:
    ; Save all GP registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Stack layout at this point:
    ; [rsp+0..r15], rbp, rdi, rsi, rdx, rcx, rbx, rax,
    ; then vector, error, rip, cs, rflags, rsp, ss (pushed by CPU + our macros)
    mov rdi, [rsp + 15*8]   ; vector
    mov rsi, [rsp + 16*8]   ; error code
    mov rdx, [rsp + 17*8]   ; rip
    call exception_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16  ; pop vector + error code
    iretq

; Build a jump table of all 256 ISR addresses
isr_table:
%assign i 0
%rep 256
    dq isr_%+i
%assign i i+1
%endrep