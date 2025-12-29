#![no_std]
#![no_main]

use core::panic::PanicInfo;
use x86_64::instructions::hlt;

// Import bootloader_api
use bootloader_api::{BootInfo, entry_point};

entry_point!(kernel_main);

fn kernel_main(_boot_info: &'static mut BootInfo) -> ! {
    loop {
        hlt();
    }
}

// Panic handler
#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {
        hlt();
    }
}
