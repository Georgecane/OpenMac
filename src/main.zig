const std = @import("std");

export fn _start() noreturn {
    const vga_ptr = @as([*]volatile u16, @ptrFromInt(0xB8000));

    const message = "OpenMac OS is running...";
    const color: u16 = 0x0F00;

    for (message, 0..) |char, i| {
        vga_ptr[i] = color | @as(u16, char);
    }

    while (true) {
        asm volatile ("hlt");
    }
}

pub fn panic(_: []const u8, _: ?*std.builtin.StackTrace, _: ?usize) noreturn {
    while (true) {
        asm volatile ("hlt");
    }
}
