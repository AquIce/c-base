#include <base/foundation/memory/stack.h>

// --= Local Header =--

typedef struct {
	usize previous_offset;
	usize current_offset;
} StackHeader;

internal void stack_init(StackCtx* stack, void* buffer, usize capacity);

internal void* stack_alloc(void* handler, usize size, usize alignment);
internal void stack_free(void* handler, void* ptr);
internal void* stack_realloc(void* handler, void* ptr, usize old_size, usize new_size);
internal void stack_reset(void* handler);

internal const AllocatorVTable stack_vtable = {
    .alloc = stack_alloc,
    .free = stack_free,
	.realloc = stack_realloc,
	.reset = stack_reset,
};

internal_fn StackHeader* compute_stack_header_ptr(void* ptr) {
	return (StackHeader*)(align_down_ptr((uptr)ptr - sizeof(StackHeader), alignof(StackHeader)));
}

// --= Implementation =--

internal void stack_init(StackCtx* stack, void* buffer, usize capacity) {
	stack->buffer = buffer;
    stack->capacity = capacity;
    stack->offset = 0;
}

internal void* stack_alloc(void* handler, usize size, usize alignment) {
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	StackCtx* stack = (StackCtx*)handler;
	usize base_offset = stack->offset;
	uptr min_header_address = align_up_ptr(base_offset, alignof(StackHeader));
	uptr min_address = (uptr)(stack->buffer + min_header_address + sizeof(StackHeader));
	uptr aligned = align_up_ptr(min_address, alignment);
	usize padding = (usize)(aligned - min_address) + sizeof(StackHeader);

	usize required = padding + size;

	if(required > stack->capacity - stack->offset) {
		return nullptr;
	}

	void* result = stack->buffer + stack->offset + padding;
	stack->offset += padding + size;
	*compute_stack_header_ptr(result) = (StackHeader) {
		.previous_offset = base_offset,
		.current_offset = stack->offset,
	};

	return result;
}

internal void stack_free(void* handler, void* ptr) {
	if(!ptr) { return; }
	StackCtx* stack = (StackCtx*)handler;
	StackHeader* header = compute_stack_header_ptr(ptr);
	
	// Only allow free on the last pointer
	assert(header->current_offset == stack->offset);
	stack->offset = header->previous_offset;
}

internal void* stack_realloc(void* handler, void* ptr, usize old_size, usize new_size) {
	if(!ptr) {
		return nullptr;
	}
	if(new_size == old_size) {
		return ptr;
	}
	if(new_size == 0) {
		stack_free(handler, ptr);
		return nullptr;
	}
	StackCtx* stack = (StackCtx*)handler;
	StackHeader* header = compute_stack_header_ptr(ptr);

	// Only allow realloc on the last allocated pointer
	assert(header->current_offset == stack->offset);
	assert(stack->offset <= stack->capacity + old_size - new_size);

	stack->offset += new_size - old_size;
	header->current_offset = stack->offset;

	return ptr;
}

internal void stack_reset(void* handler) {
	((StackCtx*)handler)->offset = 0;
}

Allocator stack_create(const MemorySource* source, usize capacity) {
	StackCtx* stack = memory_source_reserve(source, sizeof(StackCtx), alignof(StackCtx), 0);
	if(!stack) {
		return (Allocator){0};
	}

	void* buffer = memory_source_reserve(source, capacity, MAX_ALIGNMENT, 0);
	if(!buffer) {
		memory_source_release(source, stack, sizeof(*stack));
		return (Allocator){0};
	}
	stack_init(stack, buffer, capacity);

    return (Allocator){
		.handler = stack,
		.vt = &stack_vtable,
		.source = source,
	};
}

void stack_destroy(Allocator* allocator) {
	if(!allocator || !allocator->handler) {
        return;
    }
	StackCtx* stack = (StackCtx*)allocator->handler;

	memory_source_release(allocator->source, stack->buffer, stack->capacity);
	memory_source_release(allocator->source, stack, sizeof(*stack));

    allocator->handler = nullptr;
    allocator->vt = nullptr;
	allocator->source = nullptr;
}
