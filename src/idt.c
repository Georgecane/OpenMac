#include "kernel.h"

idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// تابع اسمبلی برای بارگذاری IDT
extern void idt_flush(uint32_t);
extern void pic_remap();

extern void irq0();
extern void irq1();

extern void isr80(); // هندلر سیستم‌کال در اسمبلی

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

void init_idt() {
    pic_remap();
    
    // وقفه‌های سخت‌افزاری
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);

    // --- وقفه سیستم‌کال OpenMac (0x80) ---
    // سطح دسترسی 0xEE یعنی User Mode مجاز است (DPL=3)
    // idt_set_gate(128, (uint32_t)isr80, 0x08, 0xEE); 

    idt_ptr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;
    idt_flush((uint32_t)&idt_ptr);

    __asm__ __volatile__("sti");
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}
