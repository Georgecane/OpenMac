#include "kernel.h"

// پورت‌های پایه برای درایور اول (Primary Bus)
#define ATA_DATA        0x1F0
#define ATA_FEATURES    0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE_SEL   0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7

// وضعیت‌های دیسک
#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01

static void ata_wait_bsy() {
    while (inb(ATA_STATUS) & ATA_STATUS_BSY);
}

static void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ));
}

// تابع خواندن سکتور (LBA 28-bit)
void read_sectors_ata(uint32_t LBA, uint8_t sector_count, uint16_t* buffer) {
    ata_wait_bsy();
    
    outb(ATA_DRIVE_SEL, (LBA >> 24) | 0xE0); // Master Drive + LBA high bits
    outb(ATA_SECTOR_CNT, sector_count);
    outb(ATA_LBA_LOW,  (uint8_t)LBA);
    outb(ATA_LBA_MID,  (uint8_t)(LBA >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(LBA >> 16));
    outb(ATA_COMMAND,  0x20); // دستور READ

    for (int j = 0; j < sector_count; j++) {
        ata_wait_bsy();
        ata_wait_drq();

        for (int i = 0; i < 256; i++) {
            buffer[i + (j * 256)] = inw(ATA_DATA);
        }
    }
}

// تابع نوشتن سکتور (بسیار مهم برای ساخت فایل)
void write_sectors_ata(uint32_t LBA, uint8_t sector_count, uint16_t* buffer) {
    ata_wait_bsy();

    outb(ATA_DRIVE_SEL, (LBA >> 24) | 0xE0);
    outb(ATA_SECTOR_CNT, sector_count);
    outb(ATA_LBA_LOW,  (uint8_t)LBA);
    outb(ATA_LBA_MID,  (uint8_t)(LBA >> 8));
    outb(ATA_LBA_HIGH, (uint8_t)(LBA >> 16));
    outb(ATA_COMMAND,  0x30); // دستور WRITE

    for (int j = 0; j < sector_count; j++) {
        ata_wait_bsy();
        ata_wait_drq();

        for (int i = 0; i < 256; i++) {
            outw(ATA_DATA, buffer[i + (j * 256)]);
        }
        
        // ارسال دستور Flush برای اطمینان از فیزیکی نوشته شدن داده
        outb(ATA_COMMAND, 0xE7); 
    }
}