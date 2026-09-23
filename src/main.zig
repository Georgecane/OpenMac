const std = @import("std");
const uefi = std.os.uefi;

const BootInfo = @import("boot/boot_info.zig").BootInfo;
const debug = @import("drivers/debug.zig");
const serial = @import("drivers/serial.zig");
const kernel = @import("kernel/kernel.zig");

fn fatal() noreturn {
    debug.write("UEFI:fatal\n");
    while (true) {
        asm volatile ("hlt");
    }
}

/// Explicit UEFI entry point.
///
/// Zig's standard startup normally exports this symbol for a UEFI target.
/// Defining it here makes the firmware entry ABI explicit and removes the
/// startup shim as a variable while we bootstrap OpenMac.
pub export fn EfiMain(
    handle: uefi.Handle,
    system_table: *uefi.tables.SystemTable,
) callconv(.c) usize {
    uefi.handle = handle;
    uefi.system_table = system_table;

    debug.write("UEFI:EfiMain\n");
    main();

    return 0;
}

pub fn main() void {
    serial.init();
    serial.write("UEFI:entered\r\n");
    debug.write("UEFI:entered\n");

    const boot_services = uefi.system_table.boot_services orelse fatal();
    debug.write("UEFI:boot-services\n");

    const map_info = boot_services.getMemoryMapInfo() catch fatal();
    debug.write("UEFI:memory-map-info\n");

    const extra_descriptors: usize = 8;
    const map_bytes = (map_info.len + extra_descriptors) * map_info.descriptor_size;

    const map_buffer = boot_services.allocatePool(.loader_data, map_bytes)
        catch fatal();
    debug.write("UEFI:map-buffer\n");

    var map = boot_services.getMemoryMap(map_buffer) catch fatal();
    debug.write("UEFI:memory-map\n");

    while (true) {
        debug.write("UEFI:exit-boot-services\n");

        boot_services.exitBootServices(uefi.handle, map.info.key) catch |err| {
            switch (err) {
                error.InvalidParameter => {
                    debug.write("UEFI:map-refresh\n");
                    map = boot_services.getMemoryMap(map_buffer)
                        catch fatal();
                    continue;
                },
                else => fatal(),
            }
        };
        break;
    }

    debug.write("UEFI:boot-services-exited\n");

    const map_size = map.info.len * map.info.descriptor_size;

    const boot_info = BootInfo{
        .memory_map = map_buffer[0..map_size],
        .memory_map_size = map_size,
        .memory_descriptor_size = map.info.descriptor_size,
        .memory_descriptor_version = map.info.descriptor_version,
    };

    debug.write("UEFI:kernel-main\n");
    kernel.main(&boot_info);
}
