const BootInfo = @import("../boot/boot_info.zig").BootInfo;
const serial = @import("../drivers/serial.zig");
const io = @import("../arch/x86_64/io.zig");

pub fn main(boot_info: *const BootInfo) noreturn {
    serial.init();

    serial.write("\r\n");
    serial.write("OpenMac kernel entered.\r\n");
    serial.write("Boot services are no longer available.\r\n");
    serial.write("Memory map bytes: ");
    serial.writeHex(boot_info.memory_map.len);
    serial.write("\r\n");
    serial.write("Descriptor size: ");
    serial.writeHex(boot_info.memory_descriptor_size);
    serial.write("\r\n");
    serial.write("Kernel idle loop reached.\r\n");

    io.halt();
}
