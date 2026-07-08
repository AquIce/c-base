#include <base/foundation/macros.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/pool.h>

#include <assert.h>

// --= Local Header =--

union PoolBlock {
    PoolBlock* next;
	void* data;
};

internal void pool_init(PoolCtx* pool, void* buffer, usize block_size, usize block_count, usize alignment, bool owns_buffer);

internal void* pool_alloc(void* handler, usize size, usize alignment);
internal void pool_free(void* handler, void* ptr);
internal void* pool_realloc(void* handler, void* ptr, usize old_size, usize new_size);
internal void pool_reset(void* handler);

internal const AllocatorVTable pool_vtable = {
    .alloc = pool_alloc,
    .free = pool_free,
	.realloc = pool_realloc,
	.reset = pool_reset,
};

// --= Implementation =--

internal void pool_build_free_list(PoolCtx* pool) {

	//   +---+
	//   |   v next
	// +---+---+---+---+---+
	// | A | B | C | D | E |
	// +---+---+---+---+---+
	//  ^
	//  free_list

	PoolBlock* previous = (PoolBlock*)pool->buffer;
    pool->free_list = (PoolBlock*)pool->buffer;

    for(usize i = 1; i < pool->block_count; i++) {
        PoolBlock* block = (PoolBlock*)((u8*)pool->buffer + i * pool->block_size);

        previous->next = block;
		previous = block;
    }
	previous->next = nullptr;
}

internal void pool_init(
	PoolCtx* pool,
	void* buffer,
	usize block_size,
	usize block_count,
	usize alignment,
	bool owns_buffer
) {

	assert(block_count != 0);

	pool->buffer = buffer;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->alignment = alignment;
	pool->owns_buffer = owns_buffer;

	pool_build_free_list(pool);
}

internal void* pool_alloc(void* handler, usize size, usize alignment) {
	PoolCtx* pool = (PoolCtx*)handler;

	assert(size <= pool->block_size);
	assert(alignment <= pool->alignment);

	PoolBlock* result = pool->free_list;
	if(!result) {
		return nullptr;
	}
	pool->free_list = result->next;
	result->next = nullptr;

	return result;
}

internal void pool_free(void* handler, void* ptr) {
	PoolCtx* pool = (PoolCtx*)handler;

	PoolBlock* block = (PoolBlock*)ptr;
	block->next = pool->free_list;
	pool->free_list = block;
}

internal void* pool_realloc(void* handler, void* ptr, usize old_size, usize new_size) {
	(void)handler;
	(void)ptr;
	(void)old_size;
	(void)new_size;

	return nullptr;
}

internal void pool_reset(void* handler) {
	PoolCtx* pool = (PoolCtx*)handler;
	pool_build_free_list(pool);
}

Allocator pool_create(
    const MemorySource* source,
    usize block_size,
    usize block_count,
    usize alignment
) {
    PoolCtx* pool = memory_source_reserve(source, sizeof(PoolCtx), alignof(PoolCtx), 0);
	if(!pool) {
		return (Allocator){0};
	}
	alignment = MAX(alignment, alignof(PoolBlock));
	block_size = align_up(
        MAX(block_size, sizeof(PoolBlock)),
        alignment
    );
	void* buffer = memory_source_reserve(
        source,
        block_size * block_count,
		alignment,
		0
    );
	if(!buffer) {
		memory_source_release(source, pool, sizeof(*pool));
		return (Allocator){0};
	}
	pool_init(pool, buffer, block_size, block_count, alignment, true);

	return (Allocator){
		.handler = pool,
		.vt = &pool_vtable,
		.source = source,
	};
}
Allocator pool_create_from_buffer(
	const MemorySource* source,
	void* buffer,
	usize buffer_size,
	usize block_size,
	usize alignment
) {
	alignment = MAX(alignment, alignof(PoolBlock));
	block_size = align_up(
        MAX(block_size, sizeof(PoolBlock)),
        alignment
    );
	assert(buffer_size % block_size == 0);

	PoolCtx* pool = memory_source_reserve(source, sizeof(PoolCtx), alignof(PoolCtx), 0);
	if(!pool) {
		return (Allocator){0};
	}	
	pool_init(pool, buffer, block_size, buffer_size / block_size, alignment, false);

	return (Allocator){
		.handler = pool,
		.vt = &pool_vtable,
		.source = source,
	};
}

void pool_destroy(Allocator* allocator) {
	if(!allocator || !allocator->handler) {
        return;
    }
	PoolCtx* pool = (PoolCtx*)allocator->handler;

    if(pool->owns_buffer) {
        memory_source_release(allocator->source, pool->buffer, pool->block_count * pool->block_size);
    }

	memory_source_release(allocator->source, pool, sizeof(*pool));

    allocator->handler = nullptr;
    allocator->vt = nullptr;
	allocator->source = nullptr;
}
