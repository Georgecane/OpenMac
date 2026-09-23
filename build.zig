const std = @import("std");

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});

    const target = b.resolveTargetQuery(.{
        .cpu_arch = .x86_64,
        .os_tag = .uefi,
        .abi = .msvc,
    });

    const kernel_module = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    const kernel = b.addExecutable(.{
        .name = "BOOTX64",
        .root_module = kernel_module,
    });

    const install_step = b.addInstallArtifact(kernel, .{
        .dest_dir = .{ .override = .{ .custom = "uefi/EFI/BOOT" } },
    });

    const qemu_ovmf = b.option(
        []const u8,
        "qemu-ovmf",
        "Path to OVMF firmware",
    ) orelse getDefaultOvmfPath();

    const run_cmd = b.addSystemCommand(&.{
        "qemu-system-x86_64",
        "-bios",
        qemu_ovmf,
        "-drive",
        "format=raw,file=fat:rw:zig-out/uefi",
        "-m",
        "512M",
        "-serial",
        "stdio",
        "-debugcon",
        "file:zig-out/openmac-debug.log",
        "-global",
        "isa-debugcon.iobase=0xe9",
        "-display",
        "none",
        "-no-reboot",
        "-no-shutdown",
    });
    run_cmd.step.dependOn(&install_step.step);

    const run_step = b.step("run-uefi", "Build and boot OpenMac through UEFI");
    run_step.dependOn(&run_cmd.step);

    const success = b.addSystemCommand(&.{
        "echo",
        "OpenMac UEFI image built at zig-out/uefi/EFI/BOOT/BOOTX64.EFI",
    });
    success.step.dependOn(&install_step.step);

    b.default_step.dependOn(&success.step);
}

fn getDefaultOvmfPath() []const u8 {
    const os = @import("builtin").os.tag;

    return switch (os) {
        .windows => "C:\\Program Files\\qemu\\share\\edk2-x86_64\\OVMF.fd",
        .linux => "/usr/share/ovmf/OVMF.fd",
        .macos => "/opt/homebrew/share/qemu/edk2-x86_64/OVMF.fd",
        else => "/usr/share/ovmf/OVMF.fd",
    };
}
