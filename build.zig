const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const kernel_module = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = target,
        .optimize = optimize,
    });

    const kernel = b.addExecutable(.{
        .name = "openmac",
        .root_module = kernel_module,
    });

    // =====================
    // Linker script - Zig 0.16 compatible
    // =====================
    kernel.root_module.addLinkerArg("-T");
    kernel.root_module.addLinkerArg("linker/uefi.ld");

    const install_step = b.addInstallArtifact(kernel, .{
        .dest_dir = .{ .override = .{ .custom = "uefi" } },
    });

    // =====================
    // Run with QEMU
    // =====================
    const qemu_ovmf = b.option(
        []const u8,
        "qemu-ovmf",
        "Path to OVMF.fd",
    ) orelse getDefaultOvmfPath();

    const run_cmd = b.addSystemCommand(&.{
        "qemu-system-x86_64",
        "-bios",
        qemu_ovmf,
        "-drive",
        b.fmt("if=pflash,format=raw,readonly=on,file={s}", .{qemu_ovmf}),
        "-drive",
        "format=raw,file=zig-out/uefi/openmac.efi",
        "-m",
        "512M",
        "-serial",
        "stdio",
        "-no-reboot",
        "-no-shutdown",
    });
    run_cmd.step.dependOn(&install_step.step);

    const run_step = b.step("run-uefi", "Run OpenMac with UEFI");
    run_step.dependOn(&run_cmd.step);

    // Success message
    const success = b.addSystemCommand(&.{
        "echo",
        "✅ OpenMac built successfully! Output: zig-out/uefi/openmac.efi",
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
