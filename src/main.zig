const std = @import("std");
const uefi = std.os.uefi;

pub fn main() void {
    // OpenMac Kernel Entry Point (UEFI)
    const con_out = uefi.system_table.con_out.?;

    _ = con_out.outputString(std.unicode.utf8ToUtf16LeStringLiteral("OpenMac Kernel Booted Successfully!\r\n"));
    _ = con_out.outputString(std.unicode.utf8ToUtf16LeStringLiteral("Welcome to OpenMachine (OpenMac)\r\n"));
    _ = con_out.outputString(std.unicode.utf8ToUtf16LeStringLiteral("UEFI Native Mode - 2026\r\n"));

    // Halt the system (infinite loop)
    while (true) {
        asm volatile ("hlt");
    }
}
