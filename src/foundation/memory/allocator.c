#include <base/foundation/memory/allocator.h>

void* allocator_alloc(const Allocator* allocator, usize size, usize alignment) {
	return allocator->vt->alloc(allocator, size, alignment);
}
void allocator_free(const Allocator* allocator, void* ptr) {
	allocator->vt->free(allocator, ptr);
}
void* allocator_realloc(const Allocator* allocator, void* ptr, usize old_size, usize new_size) {
	return allocator->vt->realloc(allocator, ptr, old_size, new_size);
}
void allocator_reset(const Allocator* allocator) {
	allocator->vt->reset(allocator);
}

