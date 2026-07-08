#ifndef __BASE_FOUNDATION_MEMORY_ARENA__
#define __BASE_FOUNDATION_MEMORY_ARENA__

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

#define POOL_CREATE_T(source, T, count) \
	pool_create((source), sizeof(T), (count), alignof(T))

#define POOL_CREATE_T_FROM_BUFFER(source, T, src_buffer) \
	pool_create_from_buffer((source), (src_buffer), sizeof(T), alignof(T))


typedef union PoolBlock PoolBlock;

typedef struct {
	PoolBlock* free_list;
	void* buffer;

	usize block_size;
	usize block_count;
	usize alignment;

	bool owns_buffer;
} PoolCtx;

Allocator pool_create(
	const MemorySource* source,
	usize block_size,
	usize block_count,
	usize alignment
);
Allocator pool_create_from_buffer(
	const MemorySource* source,
	void* buffer,
	usize buffer_size,
	usize block_size,
	usize alignment
);
void pool_destroy(Allocator* allocator);

#endif // __BASE_FOUNDATION_MEMORY_ARENA__
