@echo off
setlocal enabledelayedexpansion

set KERNEL_BIN=kernel.bin
set LINKER_SCRIPT=src/linker.ld
set DISK_IMG=disk.img

echo [1/6] Cleaning old files...
if exist *.o del *.o
if exist %KERNEL_BIN% del %KERNEL_BIN%

echo [2/6] Compiling Assembly Sources...
set "ASM_OBJ_FILES="
for %%f in (src\*.s) do (
    echo    Assembling: %%f
    nasm -f elf32 %%f -o %%~nf.o
    set "ASM_OBJ_FILES=!ASM_OBJ_FILES! %%~nf.o"
)

echo [3/6] Compiling C Sources...
set "C_OBJ_FILES="
for %%f in (src\*.c) do (
    echo    Compiling: %%f
    clang --target=i386-pc-none-elf -march=i386 -c %%f -o %%~nf.o -ffreestanding -O2 -nostdlib -Isrc
    set "C_OBJ_FILES=!C_OBJ_FILES! %%~nf.o"
)

echo [4/6] Linking Everything...
:: لینک کردن تمام فایل‌های Object تولید شده از C و اسمبلی
ld.lld -m elf_i386 -T %LINKER_SCRIPT% !ASM_OBJ_FILES! !C_OBJ_FILES! -o %KERNEL_BIN% --build-id=none

if not exist %KERNEL_BIN% (
    echo [X] Linking Failed!
    pause
    exit /b 1
)

echo [5/6] Preparing Disk...
if not exist %DISK_IMG% (
    fsutil file createnew %DISK_IMG% 10485760
)

echo [6/6] Launching QEMU...
:: استفاده از -m 32 برای تخصیص رم کافی جهت تست‌های حافظه
qemu-system-i386 -kernel %KERNEL_BIN% -drive format=raw,file=%DISK_IMG% -m 32

:done
echo Done.
pause