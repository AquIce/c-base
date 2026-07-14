#ifndef __BASE_FOUNDATION_MEMORY_STACK__
#define __BASE_FOUNDATION_MEMORY_STACK__

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

typedef struct {
    void* buffer;
    usize capacity;
    usize offset;
} StackCtx;

Allocator stack_create(const MemorySource* source, usize capacity);
void stack_destroy(Allocator* allocator);

#endif // __BASE_FOUNDATION_MEMORY_STACK__
