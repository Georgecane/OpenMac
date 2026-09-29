const BootInfo = @import("../boot/boot_info.zig").BootInfo;
const debug = @import("../drivers/debug.zig");
const serial = @import("../drivers/serial.zig");
const io = @import("../arch/x86_64/io.zig");
const gdt = @import("../arch/x86_64/gdt.zig");
const idt = @import("../arch/x86_64/idt.zig");

pub fn main(boot_info: *const BootInfo) noreturn {
    debug.write("KERNEL:entered\n");

    gdt.init();
    debug.write("KERNEL:gdt-init\n");

    idt.init();
    debug.write("KERNEL:idt-init\n");

    serial.init();
    debug.write("KERNEL:serial-init\n");

    serial.write("\r\n");
    serial.write("OpenMac kernel entered.\r\n");
    serial.write("Boot services are no longer available.\r\n");
    serial.write("Memory map bytes: ");
    serial.writeHex(boot_info.memory_map.len);
    serial.write("\r\n");
    serial.write("Descriptor size: ");
    serial.writeHex(boot_info.memory_descriptor_size);
    serial.write("\r\n");

    debug.write("KERNEL:trigger-ud\n");
    serial.write("Triggering undefined instruction exception (#UD).\r\n");
    asm volatile ("ud2");

    unreachable;
}
