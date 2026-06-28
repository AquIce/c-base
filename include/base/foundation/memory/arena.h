#ifndef __BASE_FOUNDATION_MEMORY_ARENA__
#define __BASE_FOUNDATION_MEMORY_ARENA__

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

#include <assert.h>
#include <stdlib.h>

typedef struct {
    u8* buffer;
    usize capacity;
    usize offset;
	bool owns_buffer;
} ArenaCtx;

Allocator arena_create(const MemorySource* source, usize capacity);
Allocator arena_create_from_buffer(const MemorySource* source, void* buffer, usize capacity);
void arena_destroy(Allocator* allocator);

#endif // __BASE_FOUNDATION_MEMORY_ARENA__
