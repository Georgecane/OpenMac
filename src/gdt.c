#include "kernel.h"

gdt_entry_t gdt_entries[5];
gdt_ptr_t   gdt_ptr;

// تابع کمکی برای تنظیم هر ورودی در GDT
void gdt_set_gate(int32_t num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high   = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low   = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access      = access;
}

void init_gdt() {
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    // ۱. سگمنت تهی (Null Segment) - الزامی برای پردازنده
    gdt_set_gate(0, 0, 0, 0, 0);                
    
    // ۲. سگمنت کد (Code Segment) - تمام ۴ گیگابایت حافظه را پوشش می‌دهد
    // Access: 0x9A (حلقه صفر، کد اجرایی، خواندنی)
    // Gran: 0xCF (صفحه‌بندی ۴ کیلوبایتی، حالت ۳۲ بیتی)
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); 

    // ۳. سگمنت داده (Data Segment) - تمام ۴ گیگابایت
    // Access: 0x92 (حلقه صفر، داده، خواندنی/نوشتنی)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); 

    // ۴. سگمنت کد حالت کاربر (User Mode Code) - برای آینده که برنامه اجرا می‌کنید
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); 

    // ۵. سگمنت داده حالت کاربر (User Mode Data)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); 

    // فراخوانی تابع اسمبلی برای بارگذاری نهایی
    extern void gdt_flush(uint32_t);
    gdt_flush((uint32_t)&gdt_ptr);
}