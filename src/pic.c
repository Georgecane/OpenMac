#include "kernel.h"

void pic_remap() {
    outb(0x20, 0x11); // شروع مقداردهی PIC اصلی
    outb(0xA0, 0x11); // شروع مقداردهی PIC فرعی
    outb(0x21, 0x20); // تغییر آدرس IRQ0 به وقفه ۳۲
    outb(0xA1, 0x28); // تغییر آدرس IRQ8 به وقفه ۴۰
    outb(0x21, 0x04); // اتصال دو PIC به هم
    outb(0xA1, 0x02);
    outb(0x21, 0x01); // حالت 8086
    outb(0xA1, 0x01);
    outb(0x21, 0x0);  // باز کردن تمام ماسک‌ها
    outb(0xA1, 0x0);
}