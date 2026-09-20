const std = @import("std");
const uefi = std.os.uefi;

const BootInfo = @import("boot/boot_info.zig").BootInfo;
const kernel = @import("kernel/kernel.zig");

fn fatal(message: []const u8) noreturn {
    const con_out = uefi.system_table.con_out orelse unreachable;
    _ = con_out.outputString(std.unicode.utf8ToUtf16LeStringLiteral("OpenMac boot failure.\r\n"));
    _ = con_out.outputString(std.unicode.utf8ToUtf16LeStringLiteral("Check the serial console for details.\r\n"));
    _ = message;
    while (true) {
        asm volatile ("hlt");
    }
}

pub fn main() void {
    const boot_services = uefi.system_table.boot_services orelse fatal("boot services unavailable");

    const map_info = boot_services.getMemoryMapInfo() catch fatal("GetMemoryMapInfo failed");
    const extra_descriptors: usize = 8;
    const map_bytes = (map_info.len + extra_descriptors) * map_info.descriptor_size;

    const map_buffer = boot_services.allocatePool(.loader_data, map_bytes)
        catch fatal("memory-map allocation failed");

    var map = boot_services.getMemoryMap(map_buffer) catch fatal("GetMemoryMap failed");

    while (true) {
        boot_services.exitBootServices(uefi.handle, map.info.key) catch |err| {
            switch (err) {
                error.InvalidParameter => {
                    map = boot_services.getMemoryMap(map_buffer)
                        catch fatal("memory-map refresh failed");
                    continue;
                },
                else => fatal("ExitBootServices failed"),
            }
        };
        break;
    }

    const boot_info = BootInfo{
        .memory_map = map_buffer,
        .memory_map_key = map.info.key,
        .memory_descriptor_size = map.info.descriptor_size,
        .memory_descriptor_version = map.info.descriptor_version,
    };

    kernel.main(&boot_info);
}
