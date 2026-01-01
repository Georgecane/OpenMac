#include "kernel.h"

// --- اعلان توابع خارجی ---
extern void list_files(int show_hidden);
extern void create_file(char* name);
extern void remove_file(char* name); // پارامتر دوم force حذف شد
extern void cat_file(char* name);
extern void write_file(char* name, char* content);
extern void make_directory(char* name);    // این را اضافه کن
extern void change_directory(char* name);  // این را اضافه کن
extern void format_disk();
extern void init_memory();
extern void* kmalloc(unsigned int size);
extern void kfree(void* ptr);

// --- تنظیمات شل و تاریخچه ---
#define MAX_HISTORY 10
#define PROMPT_STR "  localhost:~/ > "
#define PROMPT_LEN 17

char history[MAX_HISTORY][256];
int history_count = 0;
int history_index = -1;

char command_buffer[256];
int command_index = 0;
int cursor_pos = 0; 
int cursor_x = 0, cursor_y = 0;

// --- توابع کمکی رشته ---
int str_compare(char* s1, char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int starts_with(char* buf, char* cmd) {
    int i = 0;
    while (cmd[i]) {
        if (buf[i] != cmd[i]) return 0;
        i++;
    }
    return 1;
}

// --- توابع کمکی گرافیکی ---
void update_cursor() {
    uint16_t pos = (cursor_y * MAX_COLS) + cursor_x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void redraw_command_line() {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    int base_pos = cursor_y * MAX_COLS + PROMPT_LEN;
    for (int i = 0; i < MAX_COLS - PROMPT_LEN; i++) 
        video_mem[base_pos + i] = (WHITE_ON_BLACK << 8) | ' ';
    for (int i = 0; i < command_index; i++) 
        video_mem[base_pos + i] = (WHITE_ON_BLACK << 8) | command_buffer[i];
    cursor_x = PROMPT_LEN + cursor_pos;
    update_cursor();
}

void clear_screen() {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) 
        video_mem[i] = (WHITE_ON_BLACK << 8) | ' ';
    cursor_x = 0; cursor_y = 0; update_cursor();
}

void k_print(const char* str) {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') { cursor_x = 0; cursor_y++; }
        else {
            video_mem[cursor_y * MAX_COLS + cursor_x] = (WHITE_ON_BLACK << 8) | str[i];
            cursor_x++;
        }
        if (cursor_x >= MAX_COLS) { cursor_x = 0; cursor_y++; }
    }
    update_cursor();
}

// تابع کمکی برای تبدیل عدد به رشته و چاپ آن
void k_print_int(uint32_t n) {
    if (n == 0) {
        k_print("0");
        return;
    }

    char buf[11]; // حداکثر ۱۰ رقم برای uint32 + نال
    buf[10] = '\0';
    int i = 9;

    while (n > 0 && i >= 0) {
        buf[i--] = (n % 10) + '0';
        n /= 10;
    }

    k_print(&buf[i + 1]);
}

void execute_command() {
    command_buffer[command_index] = '\0';
    k_print("\n");

    // حذف فاصله‌های ابتدایی (Trim Leading Spaces)
    char* cmd = command_buffer;
    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') goto skip;

    // --- بخش دستورات فایل سیستم ---
    if (str_compare(cmd, "ls") == 0) {
        list_files(0);
    } 
    else if (starts_with(cmd, "ls -a")) {
        list_files(1);
    }
    else if (str_compare(cmd, "pwd") == 0) {
        pwd();
    }
    else if (starts_with(cmd, "cd ")) {
        change_directory(cmd + 3);
    }
    else if (starts_with(cmd, "mkdir ")) {
        make_directory(cmd + 6);
    }
    else if (starts_with(cmd, "cf ")) { // نام مستعار برای ایجاد فایل
        create_file(cmd + 6);
    }
    else if (starts_with(cmd, "rm ")) {
        remove_file(cmd + 3);
    }
    else if (starts_with(cmd, "cat ")) {
        cat_file(cmd + 4);
    }
    else if (starts_with(cmd, "write ")) {
        char* filename = cmd + 6;
        char* content = 0;
        for(int i=0; filename[i]; i++) {
            if(filename[i] == ' ') {
                filename[i] = '\0';
                content = filename + i + 1;
                break;
            }
        }
        if(content) write_file(filename, content);
        else k_print("  Usage: write [file] [text]\n");
    }
    // --- بخش دستورات سیستم ---
    else if (str_compare(cmd, "clear") == 0) {
        clear_screen();
    }
    else if (str_compare(cmd, "format") == 0) {
        format_disk();
    }
    else if (str_compare(cmd, "help") == 0) {
        k_print("  Commands: ls, pwd, cd, mkdir, cf, rm, cat, write, clear, format \n");
    }

    else {
        k_print("  Unknown command: ");
        k_print(cmd);
        k_print("\n");
    }

skip:
    command_index = 0;
    cursor_pos = 0;
    k_print(PROMPT_STR);
}

// --- مدیریت ورودی‌ها ---
void shell_handle_special(uint8_t scancode) {
    if (scancode == 0x4D) { // Right
        if (cursor_pos < command_index) { cursor_pos++; redraw_command_line(); }
    }
    else if (scancode == 0x4B) { // Left
        if (cursor_pos > 0) { cursor_pos--; redraw_command_line(); }
    }
    else if (scancode == 0x48) { // Up
        if (history_count > 0 && history_index < history_count - 1) {
            history_index++;
            int j = 0;
            while(history[history_index][j]) { 
                command_buffer[j] = history[history_index][j]; 
                j++; 
            }
            command_buffer[j] = '\0'; command_index = j; cursor_pos = j;
            redraw_command_line();
        }
    }
}

void shell_input_char(char c) {
    if (c == '\n') {
        if (command_index > 0) {
            for (int i = MAX_HISTORY - 1; i > 0; i--) {
                for (int j = 0; j < 256; j++) history[i][j] = history[i-1][j];
            }
            for (int j = 0; j < 256; j++) history[0][j] = command_buffer[j];
            if (history_count < MAX_HISTORY) history_count++;
        }
        execute_command();
    } 
    else if (c == '\b') {
        if (cursor_pos > 0) {
            for (int i = cursor_pos - 1; i < command_index; i++) command_buffer[i] = command_buffer[i+1];
            command_index--; cursor_pos--; redraw_command_line();
        }
    } 
    else if (c >= ' ' && command_index < 250) {
        for (int i = command_index; i > cursor_pos; i--) command_buffer[i] = command_buffer[i-1];
        command_buffer[cursor_pos] = c;
        command_index++; cursor_pos++; redraw_command_line();
    }
}

void kernel_main() {
    init_gdt();
    init_idt(); 
    init_memory();
    clear_screen();

    k_print("  OpenMac OS v1.2 (History & Arrows Enabled)\n");
    k_print(PROMPT_STR);

    while(1) {
        __asm__ __volatile__("hlt");
    }
}