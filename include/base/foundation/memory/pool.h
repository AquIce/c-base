#ifndef __BASE_FOUNDATION_MEMORY_POOL__
#define __BASE_FOUNDATION_MEMORY_POOL__

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

#define POOL_CREATE_T(source, T, count) \
	pool_create((source), sizeof(T), (count), alignof(T))

typedef struct PoolBlock PoolBlock;

typedef struct {
	PoolBlock* free_list;
	void* buffer;
	PoolBlock* meta_buffer;

	usize block_size;
	usize block_count;
	usize alignment;
} PoolCtx;

Allocator pool_create(
	const MemorySource* source,
	usize block_size,
	usize block_count,
	usize alignment
);
void pool_destroy(Allocator* allocator);

#endif // __BASE_FOUNDATION_MEMORY_POOL__
