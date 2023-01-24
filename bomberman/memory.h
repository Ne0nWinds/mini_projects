#include "general.h"
#include <winnt.h>

static u8* start;
static u8* current;
static u64 pages_alloced = 16;

#define PAGE_SIZE 65536

void BumpInit() {
    start = current = VirtualAlloc(NULL, (u32)-1, MEM_RESERVE, PAGE_READWRITE);
    VirtualAlloc(start, PAGE_SIZE * pages_alloced, MEM_COMMIT, PAGE_READWRITE);
}

__attribute__((alloc_size(1)))
__attribute__((assume_aligned(16)))
void *BumpAlloc(u32 size) {
    u8 *ptr = current;
    size += 15;
    size &= ~0xF;
    current += size;
    
    // static char buffer[64] = {0};
    // sprintf(buffer, "%d\n", size);
    // OutputDebugString(buffer);
    
    if (current - start > pages_alloced * PAGE_SIZE) {
        void *address = start + (u64)pages_alloced * (u64)PAGE_SIZE;
        
        u32 difference = current - start;
        size += PAGE_SIZE - 1;
        size &= ~(PAGE_SIZE - 1);
        
        void *old_address = VirtualAlloc(address, difference, MEM_COMMIT, PAGE_READWRITE);
        old_address = old_address;
    }
    
    ZeroMemory(ptr, size);

    return ptr;
}

void BumpReset() {
    current = start;
}
