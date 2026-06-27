#include <base/foundation/memory/allocator.h>

void* allocator_alloc(Allocator* allocator, usize size, usize alignment) {
	return allocator->vt->alloc(allocator->handler, size, alignment);
}

void allocator_free(Allocator* allocator, void* ptr) {
	allocator->vt->free(allocator->handler, ptr);
}
void* allocator_realloc(Allocator* allocator, void* ptr, usize old_size, usize new_size) {
	return allocator->vt->realloc(allocator->handler, ptr, old_size, new_size);
}
void allocator_reset(Allocator* allocator) {
	allocator->vt->reset(allocator->handler);
}


