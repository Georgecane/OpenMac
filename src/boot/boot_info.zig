const uefi = @import("std").os.uefi;

pub const BootInfo = struct {
    memory_map: []align(@alignOf(uefi.tables.MemoryDescriptor)) u8,
    memory_map_size: usize,
    memory_map_key: uefi.tables.MemoryMapKey,
    memory_descriptor_size: usize,
    memory_descriptor_version: u32,
};

pub const MemoryMap = struct {
    buffer: []align(@alignOf(uefi.tables.MemoryDescriptor)) u8,
    size: usize,
    descriptor_size: usize,
    descriptor_version: u32,
    key: uefi.tables.MemoryMapKey,
};
