const io = @import("../arch/x86_64/io.zig");

const COM1: u16 = 0x3F8;

pub fn init() void {
    io.out8(COM1 + 1, 0x00);
    io.out8(COM1 + 3, 0x80);
    io.out8(COM1 + 0, 0x03);
    io.out8(COM1 + 1, 0x00);
    io.out8(COM1 + 3, 0x03);
    io.out8(COM1 + 2, 0xC7);
    io.out8(COM1 + 4, 0x0B);
}

fn canWrite() bool {
    return (io.in8(COM1 + 5) & 0x20) != 0;
}

pub fn writeByte(byte: u8) void {
    while (!canWrite()) {}
    io.out8(COM1, byte);
}

pub fn write(text: []const u8) void {
    for (text) |byte| {
        writeByte(byte);
    }
}

pub fn writeHex(value: usize) void {
    const digits = "0123456789ABCDEF";
    var shift: usize = @bitSizeOf(usize);

    write("0x");

    while (shift > 0) {
        shift -= 4;
        writeByte(digits[(value >> @intCast(shift)) & 0xF]);
    }
}
