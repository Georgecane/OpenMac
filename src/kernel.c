void kernel_main(void) {
    char* video_memory = (char*) 0xB8000;
    const char* str = "OpenMac is running with Clang!";
    
    // پاک کردن صفحه
    for (int i = 0; i < 80 * 25 * 2; i++) {
        video_memory[i] = 0;
    }

    // چاپ متن
    for (int i = 0; str[i] != '\0'; i++) {
        video_memory[i * 2] = str[i];
        video_memory[i * 2 + 1] = 0x0A; // رنگ سبز
    }
}