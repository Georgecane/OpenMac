#include "kernel.h"

// پورت‌های سخت‌افزاری ATA
#define ATA_DATA        0x1F0
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

// تنظیمات فایل‌سیستم فرضی ما
#define ROOT_DIR_SECTOR 10
#define DATA_REGION_START 20

#pragma pack(push, 1)
typedef struct {
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t create_time_ms;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_modified_time;
    uint16_t last_modified_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} directory_entry_t;
#pragma pack(pop)

uint16_t fs_buffer[256];

// توابع پایه درایور
static void ata_wait_bsy() { while (inb(0x1F7) & 0x80); }
static void ata_wait_drq() { while (!(inb(0x1F7) & 0x08)); }

void read_sectors_ata(uint32_t LBA, uint8_t sector_count, uint16_t* buffer) {
    ata_wait_bsy();
    outb(0x1F6, (LBA >> 24) | 0xE0);
    outb(0x1F2, sector_count);
    outb(0x1F3, (uint8_t)LBA);
    outb(0x1F4, (uint8_t)(LBA >> 8));
    outb(0x1F5, (uint8_t)(LBA >> 16));
    outb(0x1F7, 0x20);
    for (int j = 0; j < sector_count; j++) {
        ata_wait_bsy(); ata_wait_drq();
        for (int i = 0; i < 256; i++) buffer[i + (j * 256)] = inw(0x1F0);
    }
}

void write_sectors_ata(uint32_t LBA, uint8_t sector_count, uint16_t* buffer) {
    ata_wait_bsy();
    outb(0x1F6, (LBA >> 24) | 0xE0);
    outb(0x1F2, sector_count);
    outb(0x1F3, (uint8_t)LBA);
    outb(0x1F4, (uint8_t)(LBA >> 8));
    outb(0x1F5, (uint8_t)(LBA >> 16));
    outb(0x1F7, 0x30);
    for (int j = 0; j < sector_count; j++) {
        ata_wait_bsy(); ata_wait_drq();
        for (int i = 0; i < 256; i++) outw(0x1F0, buffer[i + (j * 256)]);
        outb(0x1F7, 0xE7);
    }
}


void list_files() {
    read_sectors_ata(ROOT_DIR_SECTOR, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;
    k_print("  TYPE  NAME      CLUSTER   SIZE\n");
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0) break;
        if ((uint8_t)entries[i].name[0] == 0xE5) continue;
        k_print(entries[i].attributes & 0x10 ? "  DIR   " : "  FILE  ");
        for(int j=0; j<8; j++) if(entries[i].name[j] != ' ') { char c[2]={entries[i].name[j],0}; k_print(c); }
        k_print("\n");
    }
}

void create_file(char* name) {
    read_sectors_ata(ROOT_DIR_SECTOR, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0 || (uint8_t)entries[i].name[0] == 0xE5) {
            for(int j=0; j<8; j++) entries[i].name[j] = (j < 8 && name[j] && name[j] != ' ') ? name[j] : ' ';
            entries[i].attributes = 0x20;
            entries[i].first_cluster_low = i + 1; // اختصاص کلاستر ساده
            entries[i].file_size = 0;
            write_sectors_ata(ROOT_DIR_SECTOR, 1, fs_buffer);
            k_print("  File Created.\n");
            return;
        }
    }
}

void remove_file(char* name) {
    read_sectors_ata(ROOT_DIR_SECTOR, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0) break;
        int match = 1;
        for(int j=0; j<8; j++) {
            if(name[j] == 0 || name[j] == ' ') break;
            if(entries[i].name[j] != name[j]) { match = 0; break; }
        }
        if (match) {
            entries[i].name[0] = 0xE5;
            write_sectors_ata(ROOT_DIR_SECTOR, 1, fs_buffer);
            k_print("  File Removed.\n");
            return;
        }
    }
    k_print("  File not found.\n");
}