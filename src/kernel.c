#include "kernel.h"

extern void list_files();
extern void create_file(char* name);
extern void remove_file(char* name);

int cursor_x = 0, cursor_y = 0;
char command_buffer[256];
int command_index = 0;

unsigned char scancode_map[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// مقایسه رشته‌ها
int str_compare(char* s1, char* s2) {
    int i = 0;
    while (s1[i] && s2[i] && s1[i] == s2[i]) i++;
    return s1[i] - s2[i];
}

// چک کردن شروع دستور
int starts_with(char* buf, char* cmd) {
    int i = 0;
    while (cmd[i]) {
        if (buf[i] != cmd[i]) return 0;
        i++;
    }
    return 1;
}

void update_cursor() {
    uint16_t pos = (cursor_y * MAX_COLS) + cursor_x;
    outb(0x3D4, 0x0F); outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E); outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear_screen() {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) video_mem[i] = (0x0F << 8) | ' ';
    cursor_x = 0; cursor_y = 0; update_cursor();
}

void k_print(const char* str) {
    uint16_t *video_mem = (uint16_t *)VIDEO_ADDR;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') { cursor_x = 0; cursor_y++; }
        else {
            video_mem[cursor_y * MAX_COLS + cursor_x] = (0x0F << 8) | str[i];
            cursor_x++;
        }
        if (cursor_x >= MAX_COLS) { cursor_x = 0; cursor_y++; }
    }
    update_cursor();
}

void execute_command() {
    command_buffer[command_index] = '\0';
    k_print("\n");

    if (str_compare(command_buffer, "ls") == 0) {
        list_files();
    } else if (starts_with(command_buffer, "cf ")) {
        create_file(command_buffer + 3);
    } else if (starts_with(command_buffer, "rm ")) {
        remove_file(command_buffer + 3);
    } else if (str_compare(command_buffer, "clear") == 0) {
        clear_screen();
    } else if (command_index > 0) {
        k_print("  Unknown command.\n");
    }

    command_index = 0;
    k_print("  localhost:~/ > ");
}

void kernel_main() {
    clear_screen();
    k_print("  OpenMac Kernel 1.0.0 (FAT32 Enabled)\n  localhost:~/ > ");

    uint8_t last_scancode = 0;
    while(1) {
        uint8_t scancode = inb(0x60);
        if (scancode != last_scancode && scancode < 0x80) {
            char c = scancode_map[scancode];
            if (c == '\b' && cursor_x > 18) {
                cursor_x--;
                uint16_t *v = (uint16_t *)VIDEO_ADDR;
                v[cursor_y * MAX_COLS + cursor_x] = (0x0F << 8) | ' ';
                if (command_index > 0) command_index--;
                update_cursor();
            } else if (c == '\n') {
                execute_command();
            } else if (c >= ' ' && command_index < 250) {
                command_buffer[command_index++] = c;
                char s[2] = {c, 0}; k_print(s);
            }
            last_scancode = scancode;
        }
        for(volatile int i = 0; i < 10000; i++);
    }
}