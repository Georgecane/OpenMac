; در فایل src/bstack.s
section .multiboot
align 4
    dd 0x1BADB002             ; Magic number
    dd 0x00000003             ; Flags (ALIGN + MEMINFO)
    dd -(0x1BADB002 + 0x00000003) ; Checksum

section .text
global _start
_start:
    ; تنظیم پشته قبل از هر کاری
    mov esp, stack_top
    
    extern kernel_main
    call kernel_main

    ; اگر کرنل تمام شد، پردازنده را متوقف کن
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 32768 ; 16 KB پشته
stack_top: