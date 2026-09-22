const std = @import("std");
const uefi = std.os.uefi;

const BootInfo = @import("boot/boot_info.zig").BootInfo;
const kernel = @import("kernel/kernel.zig");

fn fatal() noreturn {
    const con_out = uefi.system_table.con_out orelse unreachable;
    con_out.outputString(std.unicode.utf8ToUtf16LeStringLiteral("OpenMac boot failure.\r\n")) catch {};
    while (true) {
        asm volatile ("hlt");
    }
}

pub fn main() void {
    const boot_services = uefi.system_table.boot_services orelse fatal();

    const map_info = boot_services.getMemoryMapInfo() catch fatal();
    const extra_descriptors: usize = 8;
    const map_bytes = (map_info.len + extra_descriptors) * map_info.descriptor_size;

    const map_buffer = boot_services.allocatePool(.loader_data, map_bytes)
        catch fatal();

    var map = boot_services.getMemoryMap(map_buffer) catch fatal();

    while (true) {
        boot_services.exitBootServices(uefi.handle, map.info.key) catch |err| {
            switch (err) {
                error.InvalidParameter => {
                    map = boot_services.getMemoryMap(map_buffer)
                        catch fatal();
                    continue;
                },
                else => fatal(),
            }
        };
        break;
    }

    const map_size = map.info.len * map.info.descriptor_size;

    const boot_info = BootInfo{
        .memory_map = map_buffer[0..map_size],
        .memory_map_size = map_size,
        .memory_descriptor_size = map.info.descriptor_size,
        .memory_descriptor_version = map.info.descriptor_version,
    };

    kernel.main(&boot_info);
}
