const uefi = @import("std").os.uefi;

pub const BootInfo = struct {
    memory_map: []align(@alignOf(uefi.tables.MemoryDescriptor)) u8,
    memory_map_size: usize,
    memory_descriptor_size: usize,
    memory_descriptor_version: u32,
};
