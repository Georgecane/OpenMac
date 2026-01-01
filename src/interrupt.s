[GLOBAL idt_flush]
idt_flush:
    mov eax, [esp+4]
    lidt [eax]        ; بارگذاری IDT
    ret

; تعریف یک ISR نمونه برای خطای تقسیم بر صفر (وقفه 0)
[GLOBAL isr0]
isr0:
    cli               ; غیرفعال کردن وقفه‌ها
    push byte 0       ; کد خطای نمایشی
    push byte 0       ; شماره وقفه
    jmp isr_common_stub

[EXTERN isr_handler]

isr_common_stub:
    pushad            ; ذخیره تمام ثبات‌ها (EAX, ECX, EDX, ...)
    mov ax, ds
    push eax          ; ذخیره سگمنت داده

    mov ax, 0x10      ; بارگذاری سگمنت داده کرنل
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler  ; فراخوانی مدیریت وقفه در C

    pop eax           ; بازگردانی سگمنت داده
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popad             ; بازگردانی ثبات‌ها
    add esp, 8        ; پاکسازی شماره وقفه و کد خطا
    iretd             ; بازگشت از وقفه

; تعریف یک ماکرو برای IRQها
%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push byte 0  ; کد خطای جعلی
    push byte %2 ; شماره وقفه
    jmp irq_common_stub
%endmacro

IRQ  0, 32 ; Timer
IRQ  1, 33 ; Keyboard
; ... می‌توانید تا IRQ 15 ادامه دهید

[EXTERN irq_handler]

irq_common_stub:
    pushad
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call irq_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popad
    add esp, 8     ; پاک کردن int_no و err_code
    iretd          ; بازگشت (بدون sti قبل از آن)