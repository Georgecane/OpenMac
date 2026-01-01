#include "kernel.h"

typedef struct malloc_header {
    unsigned int size;
    int is_free;
    struct malloc_header* next;
} malloc_header_t;

#define HEAP_START 0x1000000 
malloc_header_t* free_list = (malloc_header_t*)HEAP_START;

void init_memory() {
    free_list->size = 0x1000000; 
    free_list->is_free = 1;
    free_list->next = 0;
}

void* kmalloc(unsigned int size) {
    malloc_header_t* curr = free_list;
    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size > size + sizeof(malloc_header_t) + 4) {
                malloc_header_t* next_block = (malloc_header_t*)((char*)curr + sizeof(malloc_header_t) + size);
                next_block->size = curr->size - size - sizeof(malloc_header_t);
                next_block->is_free = 1;
                next_block->next = curr->next;
                curr->size = size;
                curr->next = next_block;
            }
            curr->is_free = 0;
            return (void*)((char*)curr + sizeof(malloc_header_t));
        }
        curr = curr->next;
    }
    return 0;
}

void kfree(void* ptr) {
    if (!ptr) return;
    malloc_header_t* header = (malloc_header_t*)((char*)ptr - sizeof(malloc_header_t));
    header->is_free = 1;

    // ادغام بلوک‌های خالی مجاور برای جلوگیری از تکه‌تکه شدن (Coalescing)
    malloc_header_t* curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += curr->next->size + sizeof(malloc_header_t);
            curr->next = curr->next->next;
        }
        curr = curr->next;
    }
}