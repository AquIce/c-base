#ifndef __BASE_FOUNDATION_MEMORY_STACK__
#define __BASE_FOUNDATION_MEMORY_STACK__

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

typedef struct {
    u8* buffer;
    usize capacity;
    usize offset;
	bool owns_buffer;
} StackCtx;

Allocator stack_create(const MemorySource* source, usize capacity);
Allocator stack_create_from_buffer(const MemorySource* source, void* buffer, usize capacity);
void stack_destroy(Allocator* allocator);

#endif // __BASE_FOUNDATION_MEMORY_STACK__
