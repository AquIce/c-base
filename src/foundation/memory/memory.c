#include <base/foundation/memory/memory.h>

#include <base/foundation/macros.h>

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

// --= Local Header =--

internal void stattrack_reserve(AllocatorStats* stats, usize size);
internal void stattrack_release(AllocatorStats* stats, usize size);

internal void* malloc_reserve(void* ctx, usize size, usize alignment, MemoryFlags flags);
internal void malloc_release(void* ctx, void* ptr, usize size);

internal void* cmalloc_reserve(void* ctx, usize size, usize alignment, MemoryFlags flags);
internal void cmalloc_release(void* ctx, void* ptr, usize size);

internal void* mmap_reserve(void* ctx, usize size, usize alignment, MemoryFlags flags);
internal void mmap_release(void* ctx, void* ptr, usize size);

internal const MemorySourceVTable malloc_source_vtable = {
	.reserve = &malloc_reserve,
	.release = &malloc_release,
};

internal const MemorySourceVTable cmalloc_source_vtable = {
	.reserve = &cmalloc_reserve,
	.release = &cmalloc_release,
};

internal const MemorySourceVTable mmap_source_vtable = {
	.reserve = &mmap_reserve,
	.release = &mmap_release,
};


// --= Implementation =--

internal void stattrack_reserve(AllocatorStats* stats, usize size) {
	stats->allocations++;
	stats->bytes_allocated += size;
}
internal void stattrack_release(AllocatorStats* stats, usize size) {
	stats->frees++;
	stats->bytes_freed += size;
}

internal void* malloc_reserve(void* ctx, usize size, usize alignment, MemoryFlags flags) {
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	(void)flags;
	MallocCtx* malloc_ctx = (MallocCtx*)ctx;

	if(alignment <= MAX_ALIGNMENT) {
		void* ptr = malloc(size);
		if(!ptr) { return nullptr; }

		stattrack_reserve(&malloc_ctx->stats, size);
		return ptr;
	}

	size = align_up(size, alignment);

	void* ptr = aligned_alloc(alignment, size);
	if(!ptr) { return nullptr; }

	stattrack_reserve(&malloc_ctx->stats, size);
	return ptr;
}
internal void malloc_release(void* ctx, void* ptr, usize size) {
	if(!ptr) { return; }

	MallocCtx* malloc_ctx = (MallocCtx*)ctx;
	stattrack_release(&malloc_ctx->stats, size);

	free(ptr);
}

internal void* cmalloc_reserve(void* ctx, usize size, usize alignment, MemoryFlags flags) {
	assert(alignment > 0);
	assert((alignment & (alignment - 1)) == 0);

	(void)flags;

	CmallocCtx* cmalloc_ctx = (CmallocCtx*)ctx;
	stattrack_reserve(&cmalloc_ctx->stats, size);

	// Min align
	if(alignment < MAX_ALIGNMENT) {
		alignment = MAX_ALIGNMENT;
	}

	// Allocate max size
	usize max_size = size + alignment - 1 + sizeof(ManualAlignedMemoryHeader);
	void* raw = malloc(max_size);
	if(!raw) {
		return nullptr;
	}

	// Calculate aligned
	uptr min_start = (uptr)raw + sizeof(ManualAlignedMemoryHeader);
	uptr aligned = align_up_ptr(min_start, alignment);

	// Add header just before
	ManualAlignedMemoryHeader* header = (ManualAlignedMemoryHeader*)(aligned - sizeof(ManualAlignedMemoryHeader));
	header->raw = raw;

	return (void*)aligned;
}

internal void cmalloc_release(void* ctx, void* ptr, usize size) {
	if(!ptr) { return; }

	ManualAlignedMemoryHeader* header = (ManualAlignedMemoryHeader*)((uptr)ptr - sizeof(ManualAlignedMemoryHeader));

	CmallocCtx* cmalloc_ctx = (CmallocCtx*)ctx;
	stattrack_release(&cmalloc_ctx->stats, size);

	free(header->raw);
}

internal void* mmap_reserve(void* ctx, usize size, usize alignment, MemoryFlags flags) {

	(void)alignment;
	(void)flags;

	MmapCtx* mmap_ctx = (MmapCtx*)ctx;

	void* ptr = mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    );

	if(ptr == MAP_FAILED) {
		return nullptr;
	}

	stattrack_reserve(&mmap_ctx->stats, size);

	return ptr;
}

internal void mmap_release(void* ctx, void* ptr, usize size) {
	MmapCtx* mmap_ctx = (MmapCtx*)ctx;

	if(!ptr) { return; }

	if(munmap(ptr, size) == 0) {
		stattrack_release(&mmap_ctx->stats, size);
	}
}

MemorySource malloc_memory_source_create() {
	MallocCtx* malloc_ctx = malloc(sizeof(MallocCtx));
	if(!malloc_ctx) {
		return (MemorySource){0};
	}

	malloc_ctx->stats = (AllocatorStats){
		.allocations = 0,
		.frees = 0,
		.bytes_allocated = 0,
		.bytes_freed = 0,
	};

	return (MemorySource){
		.ctx = malloc_ctx,
		.vt = &malloc_source_vtable,
	};
}
MemorySource cmalloc_memory_source_create() {
	CmallocCtx* cmalloc_ctx = malloc(sizeof(CmallocCtx));
	if(!cmalloc_ctx) {
		return (MemorySource){0};
	}

	cmalloc_ctx->stats = (AllocatorStats){
		.allocations = 0,
		.frees = 0,
		.bytes_allocated = 0,
		.bytes_freed = 0,
	};

	return (MemorySource){
		.ctx = cmalloc_ctx,
		.vt = &cmalloc_source_vtable,
	};
}
MemorySource mmap_memory_source_create() {
	MmapCtx* mmap_ctx = malloc(sizeof(MmapCtx));
	if(!mmap_ctx) {
		return (MemorySource){0};
	}

	mmap_ctx->stats = (AllocatorStats){
		.allocations = 0,
		.frees = 0,
		.bytes_allocated = 0,
		.bytes_freed = 0,
	};
	mmap_ctx->flags = 0;

	return (MemorySource){
		.ctx = mmap_ctx,
		.vt = &mmap_source_vtable,
	};
}

void malloc_memory_source_destroy(MemorySource *malloc_source) {
	if(!malloc_source) { return; }
	free(malloc_source->ctx);
	*malloc_source = (MemorySource){0};
}
void cmalloc_memory_source_destroy(MemorySource *cmalloc_source) {
	if(!cmalloc_source) { return; }
	free(cmalloc_source->ctx);
	*cmalloc_source = (MemorySource){0};
}
void mmap_memory_source_destroy(MemorySource *mmap_source) {
	if(!mmap_source) { return; }
	free(mmap_source->ctx);
	*mmap_source = (MemorySource){0};
}

void* memory_source_reserve(
	const MemorySource* source,
	usize size,
	usize alignment,
	MemoryFlags flags
) {
	return source->vt->reserve(
		source->ctx,
		size,
		alignment,
		flags
	);
}
void memory_source_release(
	const MemorySource* source,
	void* ptr,
	usize size
) {
	source->vt->release(
		source->ctx,
		ptr,
		size
	);
}
