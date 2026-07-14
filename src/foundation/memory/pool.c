#include <base/foundation/memory/pool.h>

#include <base/foundation/macros.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/memory.h>

#include <assert.h>

// --= Local Header =--

struct PoolBlock {
    PoolBlock* next;
};

internal void pool_init(
	PoolCtx* pool,
	void* buffer,
	PoolBlock* meta_buffer,
	usize block_size,
	usize block_count,
	usize alignment
);

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

	PoolBlock* previous = pool->meta_buffer;
    pool->free_list = pool->meta_buffer;

    for(usize i = 1; i < pool->block_count; i++) {
        PoolBlock* block = pool->meta_buffer + i;

        previous->next = block;
		previous = block;
    }
	previous->next = nullptr;
}

internal void pool_init(
	PoolCtx* pool,
	void* buffer,
	PoolBlock* meta_buffer,
	usize block_size,
	usize block_count,
	usize alignment
) {
	assert(block_count != 0);

	pool->buffer = buffer;
	pool->meta_buffer = meta_buffer;
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->alignment = alignment;

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

	usize pool_index = (usize)(result - pool->meta_buffer);
	return (void*)((u8*)pool->buffer + pool_index * pool->block_size);
}

internal void pool_free(void* handler, void* ptr) {
	PoolCtx* pool = (PoolCtx*)handler;

	assert(ptr >= pool->buffer);
	assert((u8*)ptr < (u8*)pool->buffer + pool->block_size * pool->block_count);
	assert(((u8*)ptr - (u8*)pool->buffer) % pool->block_size == 0);

	usize pool_index = (uptr)((u8*)ptr - (u8*)pool->buffer) / pool->block_size;

	PoolBlock* block = (PoolBlock*)(pool->meta_buffer + pool_index);
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
	assert(alignment != 0);
	assert((alignment & (alignment - 1)) == 0);

	block_size = align_up(block_size, alignment);

	usize meta_buffer_size = sizeof(PoolCtx) + sizeof(PoolBlock) * block_count;
    void* meta_buffer = memory_source_reserve(
		source,
		meta_buffer_size,
		alignof(PoolCtx), // WARN: Works because `alignof(PoolCtx) % alignof(PoolBlock) == 0` (`PoolBlock` is just a pointer in a struct).
		0
	);
	if(!meta_buffer) {
		return (Allocator){0};
	}
	PoolCtx* pool = (PoolCtx*)meta_buffer;

	usize buffer_size = block_size * block_count;
	void* buffer = memory_source_reserve(
        source,
        buffer_size,
		alignment,
		0
    );
	if(!buffer) {
		memory_source_release(source, meta_buffer, meta_buffer_size);
		return (Allocator){0};
	}

	usize header_size = align_up(sizeof(PoolCtx), alignof(PoolBlock));

	pool_init(
		pool,
		buffer,
		(PoolBlock*)((u8*)pool + header_size),
		block_size,
		block_count,
		alignment
	);

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

	memory_source_release(allocator->source, pool->buffer, pool->block_count * pool->block_size);
	memory_source_release(allocator->source, pool, sizeof(PoolCtx) + sizeof(PoolBlock) * pool->block_count);

    allocator->handler = nullptr;
    allocator->vt = nullptr;
	allocator->source = nullptr;
}
