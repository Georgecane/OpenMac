#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define VIDEO_ADDR 0xB8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0F

#ifdef __INTELLISENSE__
    #define __asm__(x)
    #define volatile
#endif

// توابع پورت
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}

// اضافه کردن خواندن ۱۶ بیتی برای دیسک
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// اضافه کردن به انتهای بخش توابع پورت در kernel.h
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ __volatile__("outw %0, %1" : : "a"(val), "Nd"(port));
}

#endif