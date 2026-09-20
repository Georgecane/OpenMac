pub inline fn out8(port: u16, value: u8) void {
    asm volatile ("outb %[value], %[port]"
        :
        : [value] "{al}" (value),
          [port] "{dx}" (port)
    );
}

pub inline fn in8(port: u16) u8 {
    return asm volatile ("inb %[port], %[value]"
        : [value] "={al}" (-> u8),
        : [port] "{dx}" (port)
    );
}

pub inline fn halt() noreturn {
    while (true) {
        asm volatile ("hlt");
    }
}
