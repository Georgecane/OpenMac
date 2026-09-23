const io = @import("../arch/x86_64/io.zig");

const DEBUG_PORT: u16 = 0xE9;

pub fn writeByte(byte: u8) void {
    io.out8(DEBUG_PORT, byte);
}

pub fn write(text: []const u8) void {
    for (text) |byte| {
        writeByte(byte);
    }
}
