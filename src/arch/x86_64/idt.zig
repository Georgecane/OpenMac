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

pub fn undefinedInstructionHandler() callconv(.naked) noreturn {
    asm volatile (
        \movw $0xE9, %%dx
        \movb $75, %%al
        \outb %%al, %%dx
        \movb $69, %%al
        \outb %%al, %%dx
        \movb $82, %%al
        \outb %%al, %%dx
        \movb $78, %%al
        \outb %%al, %%dx
        \movb $69, %%al
        \outb %%al, %%dx
        \movb $76, %%al
        \outb %%al, %%dx
        \movb $58, %%al
        \outb %%al, %%dx
        \movb $101, %%al
        \outb %%al, %%dx
        \movb $120, %%al
        \outb %%al, %%dx
        \movb $99, %%al
        \outb %%al, %%dx
        \movb $101, %%al
        \outb %%al, %%dx
        \movb $112, %%al
        \outb %%al, %%dx
        \movb $116, %%al
        \outb %%al, %%dx
        \movb $105, %%al
        \outb %%al, %%dx
        \movb $111, %%al
        \outb %%al, %%dx
        \movb $110, %%al
        \outb %%al, %%dx
        \movb $45, %%al
        \outb %%al, %%dx
        \movb $117, %%al
        \outb %%al, %%dx
        \movb $100, %%al
        \outb %%al, %%dx
        \movb $10, %%al
        \outb %%al, %%dx
        \cli
        \hlt
    );
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
