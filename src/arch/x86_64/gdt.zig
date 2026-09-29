const GdtPointer = packed struct {
    limit: u16,
    base: u64,
};

const KERNEL_CODE_SELECTOR: u16 = 0x08;
const KERNEL_DATA_SELECTOR: u16 = 0x10;

// Long-mode GDT:
// 0x00: null
// 0x08: ring-0 64-bit code
// 0x10: ring-0 data
var gdt: [3]u64 = .{
    0x0000000000000000,
    0x00AF9A000000FFFF,
    0x00AF92000000FFFF,
};

pub fn init() void {
    var pointer = GdtPointer{
        .limit = @sizeOf(@TypeOf(gdt)) - 1,
        .base = @intFromPtr(&gdt),
    };
    asm volatile ("lgdt (%[pointer])"
        :
        : [pointer] "r" (&pointer)
    );

    asm volatile (
        \\pushq %[code_selector]
        \\leaq 1f(%rip), %%rax
        \\pushq %%rax
        \\lretq
        \\1:
        :
        : [code_selector] "i" (KERNEL_CODE_SELECTOR)
        : .{ .rax = true, .memory = true }
    );

    asm volatile (
        \\movw %[data_selector], %%ax
        \\movw %%ax, %%ds
        \\movw %%ax, %%es
        \\movw %%ax, %%ss
        :
        : [data_selector] "i" (KERNEL_DATA_SELECTOR)
        : .{ .rax = true, .memory = true }
    );
}
