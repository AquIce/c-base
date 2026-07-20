#include <base/foundation/memory/arena.h>

#include <base/foundation/memory/allocator.h>

// --= Local Header =--

internal void arena_init(ArenaCtx* arena, void* buffer, usize capacity);

internal void* arena_alloc(const Allocator* alloc, usize size, usize alignment);
internal void arena_free(const Allocator* alloc, void* ptr);
internal void* arena_realloc(const Allocator* alloc, void* ptr, usize old_size, usize new_size);
internal void arena_reset(const Allocator* alloc);

internal const AllocatorVTable arena_vtable = {
    .alloc = arena_alloc,
    .free = arena_free,
	.realloc = arena_realloc,
	.reset = arena_reset,
};

// --= Implementation =--

internal void arena_init(ArenaCtx* arena, void* buffer, usize capacity) {
	arena->buffer = buffer;
    arena->capacity = capacity;
    arena->offset = 0;
}

internal void* arena_alloc(const Allocator* alloc, usize size, usize alignment) {
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	ArenaCtx* arena = (ArenaCtx*)alloc->handler;
	uptr current = (uptr)(arena->buffer + arena->offset);
	uptr aligned = align_up_ptr(current, alignment);
	usize padding = (usize)(aligned - current);

	usize required = padding + size;

	if(required > arena->capacity - arena->offset) {
		return nullptr;
	}

	arena->offset += padding;
	void* result = arena->buffer + arena->offset;
	arena->offset += size;

	return result;
}

internal void arena_free(const Allocator* alloc, void* ptr) {
	(void)alloc;
	(void)ptr;
}

internal void* arena_realloc(const Allocator* alloc, void* ptr, usize old_size, usize new_size) {
	(void)alloc;
	(void)ptr;
	(void)old_size;
	(void)new_size;

	return nullptr;
}

internal void arena_reset(const Allocator* alloc) {
	((ArenaCtx*)alloc->handler)->offset = 0;
}

Allocator arena_create(const MemorySource* source, usize capacity) {
	ArenaCtx* arena = MEMORY_SOURCE_RESERVE_T(ArenaCtx, source);
	if(!arena) {
		return (Allocator){0};
	}

	void* buffer = memory_source_reserve(source, capacity, MAX_ALIGNMENT, 0);
	if(!buffer) {
		memory_source_release(source, arena, sizeof(*arena));
		return (Allocator){0};
	}
	arena_init(arena, buffer, capacity);

    return (Allocator){
		.handler = arena,
		.vt = &arena_vtable,
		.source = source,
	};
}

void arena_destroy(Allocator* allocator) {
	if(!allocator || !allocator->handler) {
        return;
    }
	ArenaCtx* arena = (ArenaCtx*)allocator->handler;

	memory_source_release(allocator->source, arena->buffer, arena->capacity);

	memory_source_release(allocator->source, arena, sizeof(*arena));

    allocator->handler = nullptr;
    allocator->vt = nullptr;
	allocator->source = nullptr;
}
