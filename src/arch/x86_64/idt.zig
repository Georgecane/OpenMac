const IdtEntry = packed struct {
    offset_low: u16,
    selector: u16,
    ist: u8,
    attributes: u8,
    offset_mid: u16,
    offset_high: u32,
    reserved: u32,
};

const IdtPointer = packed struct {
    limit: u16,
    base: u64,
};

const KERNEL_CODE_SELECTOR: u16 = 0x08;
const PRESENT_INTERRUPT_GATE: u8 = 0x8E;

var idt: [256]IdtEntry = [_]IdtEntry{emptyEntry()} ** 256;
var pointer: IdtPointer = undefined;

fn emptyEntry() IdtEntry {
    return .{
        .offset_low = 0,
        .selector = 0,
        .ist = 0,
        .attributes = 0,
        .offset_mid = 0,
        .offset_high = 0,
        .reserved = 0,
    };
}

fn setGate(vector: u8, handler: u64) void {
    idt[vector] = .{
        .offset_low = @truncate(handler),
        .selector = KERNEL_CODE_SELECTOR,
        .ist = 0,
        .attributes = PRESENT_INTERRUPT_GATE,
        .offset_mid = @truncate(handler >> 16),
        .offset_high = @truncate(handler >> 32),
        .reserved = 0,
    };
}

fn defaultHandler() callconv(.naked) noreturn {
    asm volatile ("cli; hlt");
}

pub fn undefinedInstructionHandler() callconv(.c) noreturn {
    @import("../../drivers/debug.zig").write("KERNEL:exception-ud\n");
    @import("../../drivers/serial.zig").write("Exception: undefined instruction (#UD).\r\n");
    @import("../../arch/x86_64/io.zig").halt();
}

pub fn init() void {
    setGate(0, @intFromPtr(&defaultHandler));
    setGate(6, @intFromPtr(&undefinedInstructionHandler));

    pointer = .{
        .limit = @sizeOf(@TypeOf(idt)) - 1,
        .base = @intFromPtr(&idt),
    };

    asm volatile ("lidt (%[pointer])"
        :
        : [pointer] "r" (&pointer)
    );
}
