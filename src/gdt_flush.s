[GLOBAL gdt_flush]

gdt_flush:
    mov eax, [esp+4]  ; دریافت آدرس gdt_ptr از پارامتر تابع
    lgdt [eax]        ; بارگذاری GDT

    mov ax, 0x10      ; 0x10 آدرس سگمنت داده ما در GDT است (ورودی دوم)
    mov ds, ax        ; بروزرسانی تمام ثبات‌های سگمنت داده
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    jmp 0x08:.flush   ; پرش دور (Far Jump) برای بروزرسانی ثبات CS (سگمنت کد)
.flush:
    ret