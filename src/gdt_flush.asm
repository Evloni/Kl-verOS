global gdt_flush

; void gdt_flush(uint64_t gdtr_addr, uint16_t cs, uint16_t ds)
; rdi = &gdtr, rsi = cs selector, rdx = ds selector
gdt_flush:
    lgdt [rdi]

    ; Far return to reload CS
    push rsi
    lea  rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    mov ax, dx   ; ds selector comes in rdx (ax = low 16)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret