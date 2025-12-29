const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{
        .default_target = .{
            .cpu_arch = .x86_64,
            .os_tag = .freestanding,
            .abi = .none,
        },
    });

    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "OpenMac",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.setLinkerScript(b.path("linker.ld"));
    exe.root_module.stack_check = false;

    b.installArtifact(exe);

    const qemu_cmd = b.addSystemCommand(&.{
        "C:\\Program Files\\qemu\\qemu-system-x86_64.exe",
        "-kernel",
        "zig-out/bin/OpenMac",
    });

    qemu_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the OpenMac kernel in QEMU");
    run_step.dependOn(&qemu_cmd.step);
}
