#include <base/foundation/memory/arena.h>

// --= Local Header =--

internal void arena_init(ArenaCtx* arena, void* buffer, usize capacity, bool owns_buffer);

internal void* arena_alloc(void* handler, usize size, usize alignment);
internal void arena_free(void* handler, void* ptr);
internal void* arena_realloc(void* handler, void* ptr, usize old_size, usize new_size);
internal void arena_reset(void* handler);

internal const AllocatorVTable arena_vtable = {
    .alloc = arena_alloc,
    .free = arena_free,
	.realloc = arena_realloc,
	.reset = arena_reset,
};

// --= Implementation =--

internal void arena_init(ArenaCtx* arena, void* buffer, usize capacity, bool owns_buffer) {
	arena->buffer = buffer;
    arena->capacity = capacity;
    arena->offset = 0;
	arena->owns_buffer = owns_buffer;
}

internal void* arena_alloc(void* handler, usize size, usize alignment) {
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	ArenaCtx* arena = (ArenaCtx*)handler;
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

internal void arena_free(void* handler, void* ptr) {
	UNUSED(handler);
	UNUSED(ptr);
}

internal void* arena_realloc(void* handler, void* ptr, usize old_size, usize new_size) {
	UNUSED(handler);
	UNUSED(ptr);
	UNUSED(old_size);
	UNUSED(new_size);
	return nullptr;
}

internal void arena_reset(void* handler) {
	((ArenaCtx*)handler)->offset = 0;
}

Allocator arena_create(const MemorySource* source, usize capacity) {
	ArenaCtx* arena = memory_source_reserve(source, sizeof(ArenaCtx), alignof(ArenaCtx), 0);
	if(!arena) {
		return (Allocator){0};
	}

	void* buffer = memory_source_reserve(source, capacity, MAX_ALIGNMENT, 0);
	if(!buffer) {
		memory_source_release(source, arena, sizeof(*arena));
		return (Allocator){0};
	}
	arena_init(arena, buffer, capacity, true);

    return (Allocator){
		.handler = arena,
		.vt = &arena_vtable,
		.source = source,
	};
}

Allocator arena_create_from_buffer(const MemorySource* source, void *buffer, usize capacity) {
	ArenaCtx* arena = memory_source_reserve(source, sizeof(ArenaCtx), alignof(ArenaCtx), 0);
	if(!arena) {
		return (Allocator){0};
	}
	arena_init(arena, buffer, capacity, false);

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

    if(arena->owns_buffer) {
        memory_source_release(allocator->source, arena->buffer, arena->capacity);
    }

	memory_source_release(allocator->source, arena, sizeof(*arena));

    allocator->handler = nullptr;
    allocator->vt = nullptr;
	allocator->source = nullptr;
}
