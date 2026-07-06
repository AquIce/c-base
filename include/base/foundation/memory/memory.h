#ifndef __BASE_FOUNDATION_MEMORY__
#define __BASE_FOUNDATION_MEMORY__

#include <base/foundation/macros.h>

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>

typedef struct {
    usize allocations;
    usize frees;
    usize bytes_allocated;
    usize bytes_freed;
} AllocatorStats;

// NOTE: Not implemented yet
typedef enum {
    MEMORY_COMMIT	= 1 << 0,
    MEMORY_ZEROED	= 1 << 1,
    MEMORY_GUARD	= 1 << 2,
    MEMORY_HUGEPAGE	= 1 << 3,
} MemoryFlags;

typedef struct {
    void* (*reserve)(
		void* ctx,
		usize size,
		usize alignment,
		MemoryFlags flags
	);
    void (*release)(
		void* ctx,
		void* ptr,
		usize size
	);
} MemorySourceVTable;

typedef struct {
    void* ctx;
	const MemorySourceVTable* vt;
} MemorySource;

typedef struct {
	AllocatorStats stats;
} MallocCtx;

typedef struct {
	AllocatorStats stats;
} CmallocCtx;

typedef struct {
	AllocatorStats stats;
    int flags;
} MmapCtx;

typedef struct {
	void* raw;
} ManualAlignedMemoryHeader;

MemorySource malloc_memory_source_create();
MemorySource cmalloc_memory_source_create();
MemorySource mmap_memory_source_create();

void malloc_memory_source_destroy(MemorySource* malloc_source);
void cmalloc_memory_source_destroy(MemorySource* cmalloc_source);
void mmap_memory_source_destroy(MemorySource* mmap_source);

void* memory_source_reserve(
	const MemorySource* source,
	usize size,
	usize alignment,
	MemoryFlags flags
);
void memory_source_release(
	const MemorySource* source,
	void* ptr,
	usize size
);


internal_fn uptr align_up_ptr(uptr ptr, usize alignment) {
    return (ptr + alignment - 1) & ~(uptr)(alignment - 1);
}

internal_fn usize align_up(usize ptr, usize alignment) {
    return (ptr + alignment - 1) & ~(alignment - 1);
}

internal_fn uptr align_down_ptr(uptr ptr, usize alignment) {
    return ptr & ~(uptr)(alignment - 1);
}

internal_fn uptr align_down(usize ptr, usize alignment) {
    return ptr & ~(alignment - 1);
}

#endif // __BASE_FOUNDATION_MEMORY__
