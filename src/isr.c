#include "kernel.h"

extern void shell_input_char(char c);
extern void shell_handle_special(uint8_t scancode);

unsigned char keyboard_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

typedef struct registers {
    uint32_t ds;                  
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // ترتیب دقیق برای انطباق با pushad
    uint32_t int_no, err_code;    
    uint32_t eip, cs, eflags;     
} registers_t;

void isr_handler(registers_t regs) {
    k_print("\n [EXCEPTION] Interrupt Received: ");
    // اینجا می‌توانید بر اساس regs.int_no پیام‌های مختلف چاپ کنید
    if (regs.int_no == 0) {
        k_print("Division By Zero!\n");
    }
    for(;;); // متوقف کردن سیستم در صورت بروز خطا
}

void irq_handler(registers_t regs) {
    // ۱. ارسال EOI - بسیار حیاتی برای جلوگیری از ری‌ست
    if (regs.int_no >= 40) {
        outb(0xA0, 0x20);
    }
    outb(0x20, 0x20);

    // ۲. مدیریت وقفه کیبورد
    if (regs.int_no == 33) {
        uint8_t scancode = inb(0x60);
        
        // نکته مهم: اگر از وقفه استفاده می‌کنید، باید کدهای Polling را از kernel_main حذف کنید
        if (!(scancode & 0x80)) { // Key Down
             char c = keyboard_map[scancode];
             if (c > 0) {
                shell_input_char(c); // این تابع را در شل صدا بزنید
             }
        }
    }
    
    // ۳. مدیریت تایمر (اختیاری برای تست)
    if (regs.int_no == 32) {
        // فعلاً خالی بماند تا سیستم پایدار شود
    }
}