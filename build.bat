@echo off
setlocal enabledelayedexpansion

set KERNEL_BIN=kernel.bin
set LINKER_SCRIPT=src/linker.ld
set DISK_IMG=disk.img

echo [1/5] Cleaning old files...
if exist *.o del *.o
if exist %KERNEL_BIN% del %KERNEL_BIN%

echo [2/5] Compiling Assembly...
nasm -f elf32 src/bstack.s -o boot.o

echo [3/5] Compiling C Sources...
:: کامپایل تمام فایل‌های C موجود در پوشه src
clang --target=i386-pc-none-elf -march=i386 -c src/kernel.c -o kernel.o -ffreestanding -O2 -nostdlib -Isrc
clang --target=i386-pc-none-elf -march=i386 -c src/fsdisk.c -o fsdisk.o -ffreestanding -O2 -nostdlib -Isrc

echo [4/5] Linking Everything...
ld.lld -m elf_i386 -T %LINKER_SCRIPT% boot.o kernel.o fsdisk.o -o %KERNEL_BIN% --build-id=none

if not exist %KERNEL_BIN% (
    echo [X] Linking Failed!
    pause
    exit /b 1
)

echo [5/5] Preparing Disk and Launching...
:: ایجاد دیسک مجازی ۱۰ مگابایتی اگر وجود نداشته باشد
if not exist %DISK_IMG% (
    fsutil file createnew %DISK_IMG% 10485760
)

:: اجرای QEMU با اتصال هارد دیسک مجازی
qemu-system-i386 -kernel %KERNEL_BIN% -drive format=raw,file=%DISK_IMG%

:done
echo Done.
pause