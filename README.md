# OpenMac

OpenMac is an open-source project aiming to **reimagine and rebuild a macOS-inspired operating system from scratch** using **Zig** and **Assembly**. While the project draws **architectural inspiration from Darwin**, all code is written from the ground up, ensuring a fully independent implementation.

## Goals

- Build a minimal **bootloader and kernel**.
- Implement core **process and memory management**.
- Develop basic **file system and I/O** support.
- Create an extensible foundation for **drivers and networking**.
- Eventually provide a lightweight **graphical environment** inspired by macOS frameworks.

## Why Zig and Assembly?

- **Zig** provides low-level control, safety, and modern language features, ideal for OS development.
- **Assembly** is used for performance-critical and hardware-specific components such as boot code and context switching.

## Getting Started

Currently, OpenMac is in the **early stages of development**. Contributors are welcome to help with:

- Bootloader development
- Kernel subsystems
- Hardware drivers
- Documentation and testing

### Requirements

- x86_64 or ARM64 architecture (depending on the branch)
- Zig compiler
- NASM or other assembler (for ASM components)
- QEMU or other virtualization software for testing

## Contribution

We welcome contributions! Please adhere to the coding style and provide clear documentation for all new modules. 

## License

This project is licensed under the **Apache License 2.0**. See the [LICENSE](LICENSE) file for details.

---

*OpenMac is an ambitious learning project. Its primary goal is educational: to explore OS design and implementation inspired by macOS architecture, without using proprietary code.*
