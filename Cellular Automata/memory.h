#include "general.h"

void BumpInit();

__attribute__((alloc_size(1)))
__attribute__((assume_aligned(16)))
void *BumpAlloc(u32 size);

void BumpFree(void *ptr);
void BumpReset();
