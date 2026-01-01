#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

// تعاریف ویدیو و پورت‌ها که قبلاً داشتیم...
#define VIDEO_ADDR 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0F

// ساختار یک ورودی در جدول GDT
struct gdt_entry_struct {
    uint16_t limit_low;           // ۱۶ بیت پایین حد حافظه
    uint16_t base_low;            // ۱۶ بیت پایین آدرس پایه
    uint8_t  base_middle;         // ۸ بیت میانی آدرس پایه
    uint8_t  access;              // بایت دسترسی (امتیازات و نوع سگمنت)
    uint8_t  granularity;         // بیت‌های پرچم و ۴ بیت بالای حد حافظه
    uint8_t  base_high;           // ۸ بیت بالای آدرس پایه
} __attribute__((packed));

typedef struct gdt_entry_struct gdt_entry_t;

// ساختار اشاره‌گر GDT که به پردازنده داده می‌شود
struct gdt_ptr_struct {
    uint16_t limit;               // اندازه کل جدول GDT منهای یک
    uint32_t base;                // آدرس فیزیکی شروع جدول
} __attribute__((packed));

typedef struct gdt_ptr_struct gdt_ptr_t;

// اعلان تابع برای فراخوانی در kernel_main
void init_gdt();

// ساختار یک ورودی در IDT
struct idt_entry_struct {
    uint16_t base_lo;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_hi;
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

// ساختار اشاره‌گر IDT برای پردازنده
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

void init_idt();

// --- این بخش را حتماً اضافه کنید ---
void k_print(const char* str);
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}
// ----------------------------------
void init_memory();
void* kmalloc(unsigned int size);
void kfree(void* ptr);

int starts_with(char* buf, char* cmd);
void k_print_int(uint32_t n);

extern void make_directory(char* name);    // این را اضافه کن
extern void change_directory(char* name);  // این را اضافه کن

void pwd();
void list_files(int show_hidden);
void change_directory(char* name);

#endif