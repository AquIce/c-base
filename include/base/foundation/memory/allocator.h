#ifndef __BASE_FOUNDATION_MEMORY_ALLOCATOR__
#define __BASE_FOUNDATION_MEMORY_ALLOCATOR__

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>

#define ALLOC(allocator, T) \
	((T*)allocator_alloc((allocator), sizeof(T), alignof(T)))

#define NALLOC(allocator, T, n) \
	((T*)allocator_alloc((allocator), sizeof(T) * (n), alignof(T)))

typedef struct Allocator Allocator;

typedef struct {
    void* (*alloc)(
		const Allocator* alloc,
		usize size,
		usize alignment
	);
    void (*free)(
		const Allocator* alloc,
		void* ptr
	);
	void* (*realloc)(
		const Allocator* alloc,
		void* ptr,
		usize old_size,
		usize new_size
	);
	void (*reset)(
		const Allocator* alloc
	);
} AllocatorVTable;

struct Allocator {
    void* handler;
    const AllocatorVTable* vt;
	const MemorySource* source;
};

void* allocator_alloc(const Allocator* allocator, usize size, usize alignment);
void allocator_free(const Allocator* allocator, void* ptr);
void* allocator_realloc(const Allocator* allocator, void* ptr, usize old_size, usize new_size);
void allocator_reset(const Allocator* allocator);

#endif // __BASE_FOUNDATION_MEMORY_ALLOCATOR__
