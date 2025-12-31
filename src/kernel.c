#include "kernel.h"

// معرفی توابع موجود در fsdisk.c برای استفاده در کرنل
extern void read_sectors_ata(uint32_t LBA, uint8_t sector_count, uint16_t* buffer);
extern void write_sectors_ata(uint32_t LBA, uint8_t sector_count, uint16_t* buffer);

int cursor_x = 0;
int cursor_y = 0;
char command_buffer[256];
int command_index = 0;

unsigned char scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

int str_compare(char* s1, char* s2) {
    int i = 0;
    while (s1[i] == s2[i]) {
        if (s1[i] == '\0') return 0;
        i++;
    }
    return s1[i] - s2[i];
}

void update_cursor() {
    uint16_t pos = (cursor_y * MAX_COLS) + cursor_x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear_screen() {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) {
        video_mem[i] = (WHITE_ON_BLACK << 8) | ' ';
    }
    cursor_x = 0; cursor_y = 0;
    update_cursor();
}

void scroll() {
    if (cursor_y >= MAX_ROWS) {
        uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
        for (int i = 0; i < (MAX_ROWS - 1) * MAX_COLS; i++) {
            video_mem[i] = video_mem[i + MAX_COLS];
        }
        for (int i = (MAX_ROWS - 1) * MAX_COLS; i < MAX_ROWS * MAX_COLS; i++) {
            video_mem[i] = (WHITE_ON_BLACK << 8) | ' ';
        }
        cursor_y = MAX_ROWS - 1;
    }
}

void handle_backspace() {
    if (cursor_x > 18) {
        cursor_x--;
        uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
        video_mem[cursor_y * MAX_COLS + cursor_x] = (WHITE_ON_BLACK << 8) | ' ';
        if (command_index > 0) command_index--;
        update_cursor();
    }
}

void k_print(const char* str) {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            video_mem[cursor_y * MAX_COLS + cursor_x] = (WHITE_ON_BLACK << 8) | str[i];
            cursor_x++;
        }
        if (cursor_x >= MAX_COLS) { cursor_x = 0; cursor_y++; }
        scroll();
    }
    update_cursor();
}

void execute_command() {
    command_buffer[command_index] = '\0';
    k_print("\n");

    if (str_compare(command_buffer, "help") == 0) {
        k_print("  Commands: help, clear, about, save, read\n");
    } 
    else if (str_compare(command_buffer, "clear") == 0) {
        clear_screen();
        k_print("\n  OpenMac Kernel 1.0.0\n  localhost:~/ > ");
        command_index = 0;
        return;
    } 
    else if (str_compare(command_buffer, "about") == 0) {
        k_print("  OpenMac OS - Disk I/O Enabled.\n");
    }
    else if (str_compare(command_buffer, "save") == 0) {
        // ذخیره یک متن تستی در سکتور ۱ دیسک
        uint16_t buffer[256];
        char* msg = "Hello from Disk!";
        for(int i=0; i<256; i++) buffer[i] = 0;
        for(int i=0; msg[i] != '\0'; i++) ((char*)buffer)[i] = msg[i];

        write_sectors_ata(1, 1, buffer);
        k_print("  Success: Data 'Hello from Disk!' saved to Sector 1.\n");
    }
    else if (str_compare(command_buffer, "read") == 0) {
        // خواندن داده از سکتور ۱ دیسک
        uint16_t buffer[256];
        read_sectors_ata(1, 1, buffer);
        k_print("  Disk Content: ");
        k_print((char*)buffer);
        k_print("\n");
    }
    else if (command_index > 0) {
        k_print("  Unknown command: ");
        k_print(command_buffer);
        k_print("\n");
    }

    command_index = 0;
    k_print("  localhost:~/ > ");
}

void kernel_main() {
    clear_screen();
    k_print("\n  OpenMac Kernel 1.0.0\n");
    k_print("  Copyright (C) 2025 Apache 2.0.\n");
    k_print("  --------------------------------------------------------------------------------\n");
    k_print("\n  Type 'help' to see available commands.\n\n");
    k_print("  localhost:~/ > ");

    uint8_t last_scancode = 0;
    while(1) {
        uint8_t scancode = inb(0x60);
        if (scancode != last_scancode) {
            if (scancode < 0x80) {
                char c = scancode_map[scancode];
                if (c == '\b') {
                    handle_backspace();
                } else if (c == '\n') {
                    execute_command();
                } else if (c > 0 && command_index < 255) {
                    command_buffer[command_index++] = c;
                    char s[2] = {c, '\0'};
                    k_print(s);
                }
            }
            last_scancode = scancode;
        }
        for(volatile int i = 0; i < 5000; i++);
    }
}