#include "kernel.h"

// --- تنظیمات ثابت FAT32 ---
#define SECTOR_SIZE         512
#define FAT_BEGIN_LBA       32
#define ROOT_DIR_SECTOR     2048
#define CLUSTER_BEGIN_LBA   2049 
#define END_OF_CHAIN        0x0FFFFFFF

typedef struct {
    char name[11];
    uint8_t attr;
    uint8_t reserved;
    uint8_t creation_time_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) directory_entry_t;

static uint16_t fs_buffer[256];      
static uint16_t data_io_buffer[256]; 
static uint32_t fat_cache[128];      

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);
extern uint16_t inw(uint16_t port);
extern void outw(uint16_t port, uint16_t val);
extern int starts_with(char* buf, char* cmd);

static uint32_t current_dir_cluster = 2; // در FAT32 کلاستر ۲ معمولاً ریشه است

char current_path[256] = "/";

void reset_to_root() {
    current_dir_cluster = 2;
}

uint32_t cluster_to_lba(uint32_t cluster) {
    if (cluster <= 2) return ROOT_DIR_SECTOR;
    return CLUSTER_BEGIN_LBA + (cluster - 2);
}

// --- توابع کمکی سخت‌افزار ---

static inline void io_wait() {
    outb(0x80, 0);
}

static int ata_wait_ready() {
    for (int i = 0; i < 4; i++) { inb(0x1F7); io_wait(); }
    int timeout = 3000000;
    while (timeout--) {
        uint8_t status = inb(0x1F7);
        if (!(status & 0x80) && (status & 0x40)) return 1;
        if (status & 0x01) return 0;
    }
    return 0;
}

// --- توابع خواندن و نوشتن سکتور ATA PIO ---

void read_sectors_ata(uint32_t sector, uint8_t count, uint16_t* buf) {
    __asm__ __volatile__("cli");
    if (!ata_wait_ready()) { __asm__ __volatile__("sti"); return; }
    outb(0x1F6, 0xE0 | ((sector >> 24) & 0x0F)); io_wait();
    outb(0x1F2, count); io_wait();
    outb(0x1F3, (uint8_t)sector); io_wait();
    outb(0x1F4, (uint8_t)(sector >> 8)); io_wait();
    outb(0x1F5, (uint8_t)(sector >> 16)); io_wait();
    outb(0x1F7, 0x20); io_wait();
    for (int j = 0; j < count; j++) {
        if (!ata_wait_ready()) break;
        for (int i = 0; i < 256; i++) buf[i + (j * 256)] = inw(0x1F0);
        io_wait();
    }
    __asm__ __volatile__("sti");
}

void write_sectors_ata(uint32_t sector, uint8_t count, uint16_t* buf) {
    __asm__ __volatile__("cli");
    if (!ata_wait_ready()) { __asm__ __volatile__("sti"); return; }
    outb(0x1F6, 0xE0 | ((sector >> 24) & 0x0F)); io_wait();
    outb(0x1F2, count); io_wait();
    outb(0x1F3, (uint8_t)sector); io_wait();
    outb(0x1F4, (uint8_t)(sector >> 8)); io_wait();
    outb(0x1F5, (uint8_t)(sector >> 16)); io_wait();
    outb(0x1F7, 0x30); io_wait();
    for (int j = 0; j < count; j++) {
        if (!ata_wait_ready()) break;
        for (int i = 0; i < 256; i++) outw(0x1F0, buf[i + (j * 256)]);
        io_wait();
    }
    __asm__ __volatile__("sti");
}

// --- مدیریت FAT ---

uint32_t get_next_cluster(uint32_t current) {
    uint32_t fat_sector = FAT_BEGIN_LBA + (current / 128);
    uint32_t offset = current % 128;
    read_sectors_ata(fat_sector, 1, (uint16_t*)fat_cache);
    return fat_cache[offset];
}

void set_fat_entry(uint32_t cluster, uint32_t value) {
    uint32_t fat_sector = FAT_BEGIN_LBA + (cluster / 128);
    uint32_t offset = cluster % 128;
    read_sectors_ata(fat_sector, 1, (uint16_t*)fat_cache);
    fat_cache[offset] = value;
    write_sectors_ata(fat_sector, 1, (uint16_t*)fat_cache);
}

uint32_t find_free_cluster() {
    for (int s = 0; s < 10; s++) {
        read_sectors_ata(FAT_BEGIN_LBA + s, 1, (uint16_t*)fat_cache);
        for (int i = 0; i < 128; i++) {
            if (fat_cache[i] == 0 && (s * 128 + i) >= 2) return (s * 128 + i);
        }
    }
    return 0;
}

// --- توابع اصلی فایل سیستم ---

void list_files(int show_hidden) {
    uint32_t lba = cluster_to_lba(current_dir_cluster);
    read_sectors_ata(lba, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;

    k_print("\n Name         Type      Size\n----------------------------\n");
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0) break;
        if ((unsigned char)entries[i].name[0] == 0xE5) continue;

        k_print(" ");
        for(int j=0; j<8; j++) {
            char t[2] = {entries[i].name[j], 0}; k_print(t);
        }
        
        if (entries[i].attr & 0x10) {
            k_print("    <DIR>     --");
        } else {
            k_print("    FILE      ");
            k_print_int(entries[i].file_size);
            k_print(" B");
        }
        k_print("\n");
    }
}

void make_directory(char* name) {
    uint32_t current_lba = cluster_to_lba(current_dir_cluster);
    read_sectors_ata(current_lba, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0 || (unsigned char)entries[i].name[0] == 0xE5) {
            uint32_t new_cluster = find_free_cluster();
            if (new_cluster < 2) { k_print(" Error: Disk Full.\n"); return; }

            // تنظیم ورودی در دایرکتوری فعلی
            for(int j=0; j<11; j++) entries[i].name[j] = ' ';
            for(int j=0; j<8 && name[j]; j++) entries[i].name[j] = name[j];
            entries[i].attr = 0x10;
            entries[i].first_cluster_low = new_cluster & 0xFFFF;
            entries[i].first_cluster_high = (new_cluster >> 16) & 0xFFFF;
            entries[i].file_size = 0;

            set_fat_entry(new_cluster, END_OF_CHAIN);

            // ایجاد ساختار داخلی پوشه جدید (ورودی‌های . و ..)
            for(int j=0; j<256; j++) data_io_buffer[j] = 0;
            directory_entry_t* sub = (directory_entry_t*)data_io_buffer;
            
            // ورودی "."
            sub[0].name[0] = '.'; for(int j=1; j<11; j++) sub[0].name[j]=' ';
            sub[0].attr = 0x10;
            sub[0].first_cluster_low = new_cluster & 0xFFFF;
            sub[0].first_cluster_high = (new_cluster >> 16) & 0xFFFF;

            // ورودی ".." (اشاره به والد)
            sub[1].name[0] = '.'; sub[1].name[1] = '.'; for(int j=2; j<11; j++) sub[1].name[j]=' ';
            sub[1].attr = 0x10;
            sub[1].first_cluster_low = current_dir_cluster & 0xFFFF;
            sub[1].first_cluster_high = (current_dir_cluster >> 16) & 0xFFFF;

            write_sectors_ata(cluster_to_lba(new_cluster), 1, data_io_buffer);
            write_sectors_ata(current_lba, 1, fs_buffer);
            k_print("  Directory created.\n");
            return;
        }
    }
}

void pwd() {
    k_print("  Current Directory: ");
    k_print(current_path);
    k_print("\n");
}

void change_directory(char* name) {
    if (starts_with(name, "..")) {
        read_sectors_ata(cluster_to_lba(current_dir_cluster), 1, fs_buffer);
        directory_entry_t* entries = (directory_entry_t*)fs_buffer;
        if (entries[1].name[0] == '.' && entries[1].name[1] == '.') {
            current_dir_cluster = entries[1].first_cluster_low | (entries[1].first_cluster_high << 16);
            
            // مدیریت رشته مسیر برای برگشت به عقب
            if (current_dir_cluster == 2) {
                current_path[0] = '/'; current_path[1] = '\0';
            } else {
                int len = 0;
                while(current_path[len]) len++;
                for(int i = len - 1; i >= 0; i--) {
                    if(current_path[i] == '/') {
                        if(i == 0) current_path[1] = '\0';
                        else current_path[i] = '\0';
                        break;
                    }
                }
            }
            k_print("  Moved up.\n");
            return;
        }
    }

    read_sectors_ata(cluster_to_lba(current_dir_cluster), 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] != 0 && (unsigned char)entries[i].name[0] != 0xE5 && starts_with(entries[i].name, name)) {
            if (entries[i].attr & 0x10) {
                current_dir_cluster = entries[i].first_cluster_low | (entries[i].first_cluster_high << 16);
                
                // اضافه کردن نام پوشه جدید به مسیر متنی
                int len = 0; while(current_path[len]) len++;
                if (len > 1) current_path[len++] = '/';
                for(int j=0; j<8 && entries[i].name[j] != ' '; j++) {
                    current_path[len++] = entries[i].name[j];
                }
                current_path[len] = '\0';

                k_print("  OK.\n");
                return;
            }
        }
    }
    k_print("  Directory not found.\n");
}

void create_file(char* name) {
    uint32_t current_lba = cluster_to_lba(current_dir_cluster);
    read_sectors_ata(current_lba, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0 || (unsigned char)entries[i].name[0] == 0xE5) {
            uint32_t cluster = find_free_cluster();
            if (cluster < 2) { k_print(" Error: No space.\n"); return; }
            for(int j=0; j<11; j++) entries[i].name[j] = ' ';
            for(int j=0; j<8 && name[j]; j++) entries[i].name[j] = name[j];
            entries[i].attr = 0x20;
            entries[i].first_cluster_low = cluster & 0xFFFF;
            entries[i].first_cluster_high = (cluster >> 16) & 0xFFFF;
            entries[i].file_size = 0;
            set_fat_entry(cluster, END_OF_CHAIN);
            write_sectors_ata(current_lba, 1, fs_buffer);
            k_print("  File created.\n");
            return;
        }
    }
}

void remove_file(char* name) {
    uint32_t current_lba = cluster_to_lba(current_dir_cluster);
    read_sectors_ata(current_lba, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;

    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] == 0) break;
        if ((unsigned char)entries[i].name[0] == 0xE5) continue;

        if (starts_with(entries[i].name, name)) {
            // ۱. آزاد کردن زنجیره کلاسترها در FAT
            uint32_t current_cluster = entries[i].first_cluster_low | (entries[i].first_cluster_high << 16);
            while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
                uint32_t next_cluster = get_next_cluster(current_cluster);
                set_fat_entry(current_cluster, 0); // کلاستر آزاد شد
                current_cluster = next_cluster;
            }

            // ۲. علامت‌گذاری فایل به عنوان "پاک شده" در دایرکتوری (0xE5)
            entries[i].name[0] = 0xE5;
            write_sectors_ata(current_lba, 1, fs_buffer);
            k_print("  File removed successfully.\n");
            return;
        }
    }
    k_print("  Error: File not found.\n");
}

void write_file(char* name, char* content) {
    uint32_t current_lba = cluster_to_lba(current_dir_cluster);
    read_sectors_ata(current_lba, 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;

    for (int i = 0; i < 16; i++) {
        // پیدا کردن فایل با نام مورد نظر در پوشه فعلی
        if (entries[i].name[0] != 0 && (unsigned char)entries[i].name[0] != 0xE5 && starts_with(entries[i].name, name)) {
            
            uint32_t curr_cluster = entries[i].first_cluster_low | (entries[i].first_cluster_high << 16);
            int char_ptr = 0;

            while (content[char_ptr] != '\0') {
                for(int j=0; j<256; j++) data_io_buffer[j] = 0;
                char* buf_ptr = (char*)data_io_buffer;
                int bytes_written = 0;

                // پر کردن یک سکتور (۵۱۲ بایت)
                while (content[char_ptr] != '\0' && bytes_written < 512) {
                    buf_ptr[bytes_written++] = content[char_ptr++];
                }

                write_sectors_ata(CLUSTER_BEGIN_LBA + (curr_cluster - 2), 1, data_io_buffer);

                // اگر هنوز متن باقی مانده، کلاستر بعدی را پیدا کن
                if (content[char_ptr] != '\0') {
                    uint32_t next = find_free_cluster();
                    if (!next) { k_print(" Disk Full!\n"); return; }
                    set_fat_entry(curr_cluster, next);
                    set_fat_entry(next, END_OF_CHAIN);
                    curr_cluster = next;
                }
            }
            // آپدیت کردن سایز فایل در دایرکتوری
            entries[i].file_size = char_ptr;
            write_sectors_ata(current_lba, 1, fs_buffer);
            k_print("  Success: File updated.\n");
            return;
        }
    }
    k_print("  Error: File not found in current directory.\n");
}

void cat_file(char* name) {
    read_sectors_ata(cluster_to_lba(current_dir_cluster), 1, fs_buffer);
    directory_entry_t* entries = (directory_entry_t*)fs_buffer;
    for (int i = 0; i < 16; i++) {
        if (entries[i].name[0] != 0 && starts_with(entries[i].name, name)) {
            uint32_t curr_cluster = entries[i].first_cluster_low | (entries[i].first_cluster_high << 16);
            k_print("\n");
            while (curr_cluster >= 2 && curr_cluster < 0x0FFFFFF8) {
                read_sectors_ata(CLUSTER_BEGIN_LBA + (curr_cluster - 2), 1, data_io_buffer);
                char* content = (char*)data_io_buffer;
                for(int k = 0; k < 512; k++) {
                    if (content[k] == '\0') goto end_cat;
                    char t[2] = {content[k], 0}; k_print(t);
                }
                curr_cluster = get_next_cluster(curr_cluster);
            }
            end_cat: k_print("\n");
            return;
        }
    }
    k_print("  File not found.\n");
}

void format_disk() {
    k_print("  Formatting disk (FAT32 structure)...\n");

    // ۱. پاکسازی جدول FAT (قرار دادن ۰ در همه ورودی‌ها)
    // ما ۱۰ سکتور اول FAT را صفر می‌کنیم
    for(int j=0; j<256; j++) fat_cache[j] = 0;
    for(int s=0; s<10; s++) {
        write_sectors_ata(FAT_BEGIN_LBA + s, 1, (uint16_t*)fat_cache);
    }

    // ۲. رزرو کردن کلاسترهای سیستمی در FAT
    // کلاستر ۰ و ۱ در FAT32 رزرو هستند. کلاستر ۲ ریشه (Root) است.
    read_sectors_ata(FAT_BEGIN_LBA, 1, (uint16_t*)fat_cache);
    fat_cache[0] = 0x0FFFFFF8; // Reserved
    fat_cache[1] = 0x0FFFFFFF; // Reserved
    fat_cache[2] = 0x0FFFFFFF; // پایان زنجیره برای Root
    write_sectors_ata(FAT_BEGIN_LBA, 1, (uint16_t*)fat_cache);

    // ۳. پاکسازی سکتور ریشه (Root Directory)
    for(int j=0; j<256; j++) fs_buffer[j] = 0;
    write_sectors_ata(ROOT_DIR_SECTOR, 1, fs_buffer);

    // ۴. بازنشانی وضعیت فعلی شل به ریشه
    current_dir_cluster = 2;

    k_print("  Format complete. Root directory initialized.\n");
}