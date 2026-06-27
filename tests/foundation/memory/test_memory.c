#include <assert.h>
#include <stdio.h>

#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>

internal void test_malloc_create(void) {
    MemorySource source = malloc_memory_source_create();

    assert(source.ctx != nullptr);
    assert(source.vt != nullptr);

    malloc_memory_source_destroy(&source);

    assert(source.ctx == nullptr);
    assert(source.vt == nullptr);

    printf("[PASS] malloc create\n");
}

internal void test_cmalloc_create(void) {
    MemorySource source = cmalloc_memory_source_create();

    assert(source.ctx != nullptr);
    assert(source.vt != nullptr);

    cmalloc_memory_source_destroy(&source);

    assert(source.ctx == nullptr);
    assert(source.vt == nullptr);

    printf("[PASS] cmalloc create\n");
}

internal void test_mmap_create(void) {
    MemorySource source = mmap_memory_source_create();

    assert(source.ctx != nullptr);
    assert(source.vt != nullptr);

    mmap_memory_source_destroy(&source);

    assert(source.ctx == nullptr);
    assert(source.vt == nullptr);

    printf("[PASS] mmap create\n");
}

internal void test_malloc_alloc_free(void) {
    MemorySource source = malloc_memory_source_create();

    void* ptr = memory_source_reserve(
        &source,
        256,
        PTR_ALIGNMENT,
        0
    );

    assert(ptr != nullptr);

    memory_source_release(
        &source,
        ptr,
        256
    );

    MallocCtx* ctx = source.ctx;

    assert(ctx->stats.allocations == 1);
    assert(ctx->stats.frees == 1);
    assert(ctx->stats.bytes_allocated == 256);
    assert(ctx->stats.bytes_freed == 256);

    malloc_memory_source_destroy(&source);

    printf("[PASS] malloc alloc/free\n");
}

internal void test_cmalloc_alloc_free(void) {
    MemorySource source = cmalloc_memory_source_create();

    void* ptr = memory_source_reserve(
        &source,
        256,
        64,
        0
    );

    assert(ptr != nullptr);
    assert(((uptr)ptr % 64) == 0);

    memory_source_release(
        &source,
        ptr,
        256
    );

    CmallocCtx* ctx = source.ctx;

    assert(ctx->stats.allocations == 1);
    assert(ctx->stats.frees == 1);
    assert(ctx->stats.bytes_allocated == 256);
    assert(ctx->stats.bytes_freed == 256);

    cmalloc_memory_source_destroy(&source);

    printf("[PASS] cmalloc alloc/free\n");
}

internal void test_mmap_alloc_free(void) {
    MemorySource source = mmap_memory_source_create();

    void* ptr = memory_source_reserve(
        &source,
        KB(16),
        PTR_ALIGNMENT,
        0
    );

    assert(ptr != nullptr);

    memory_source_release(
        &source,
        ptr,
        KB(16)
    );

    MmapCtx* ctx = source.ctx;

    assert(ctx->stats.allocations == 1);
    assert(ctx->stats.frees == 1);
    assert(ctx->stats.bytes_allocated == KB(16));
    assert(ctx->stats.bytes_freed == KB(16));

    mmap_memory_source_destroy(&source);

    printf("[PASS] mmap alloc/free\n");
}

internal void test_malloc_alignment(void) {
    MemorySource source = malloc_memory_source_create();

    void* p8  = memory_source_reserve(&source, 32, 8, 0);
    void* p16 = memory_source_reserve(&source, 32, 16, 0);
    void* p32 = memory_source_reserve(&source, 32, 32, 0);
    void* p64 = memory_source_reserve(&source, 32, 64, 0);

    assert(((uptr)p8  % 8)  == 0);
    assert(((uptr)p16 % 16) == 0);
    assert(((uptr)p32 % 32) == 0);
    assert(((uptr)p64 % 64) == 0);

    memory_source_release(&source, p8, 32);
    memory_source_release(&source, p16, 32);
    memory_source_release(&source, p32, 32);
    memory_source_release(&source, p64, 64); /* aligned allocation */

    malloc_memory_source_destroy(&source);

    printf("[PASS] malloc alignment\n");
}

internal void test_cmalloc_alignment(void) {
    MemorySource source = cmalloc_memory_source_create();

    void* p8   = memory_source_reserve(&source, 32, 8, 0);
    void* p16  = memory_source_reserve(&source, 32, 16, 0);
    void* p32  = memory_source_reserve(&source, 32, 32, 0);
    void* p64  = memory_source_reserve(&source, 32, 64, 0);
    void* p128 = memory_source_reserve(&source, 32, 128, 0);

    assert(((uptr)p8   % PTR_ALIGNMENT) == 0);
    assert(((uptr)p16  % 16) == 0);
    assert(((uptr)p32  % 32) == 0);
    assert(((uptr)p64  % 64) == 0);
    assert(((uptr)p128 % 128) == 0);

    memory_source_release(&source, p8, 32);
    memory_source_release(&source, p16, 32);
    memory_source_release(&source, p32, 32);
    memory_source_release(&source, p64, 32);
    memory_source_release(&source, p128, 32);

    cmalloc_memory_source_destroy(&source);

    printf("[PASS] cmalloc alignment\n");
}

internal void test_multiple_allocations(void) {
    MemorySource source = malloc_memory_source_create();

    void* a = memory_source_reserve(&source, 64, 8, 0);
    void* b = memory_source_reserve(&source, 64, 8, 0);
    void* c = memory_source_reserve(&source, 64, 8, 0);

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);

    assert(a != b);
    assert(a != c);
    assert(b != c);

    memory_source_release(&source, a, 64);
    memory_source_release(&source, b, 64);
    memory_source_release(&source, c, 64);

    MallocCtx* ctx = source.ctx;

    assert(ctx->stats.allocations == 3);
    assert(ctx->stats.frees == 3);

    malloc_memory_source_destroy(&source);

    printf("[PASS] multiple allocations\n");
}

internal void test_null_release(void) {
    MemorySource source = malloc_memory_source_create();

    memory_source_release(
        &source,
        nullptr,
        64
    );

    MallocCtx* ctx = source.ctx;

    assert(ctx->stats.allocations == 0);
    assert(ctx->stats.frees == 0);

    malloc_memory_source_destroy(&source);

    printf("[PASS] null release\n");
}

internal void test_stats_accumulate(void) {
    MemorySource source = cmalloc_memory_source_create();

    void* a = memory_source_reserve(&source, 32, 16, 0);
    void* b = memory_source_reserve(&source, 64, 32, 0);

    memory_source_release(&source, a, 32);
    memory_source_release(&source, b, 64);

    CmallocCtx* ctx = source.ctx;

    assert(ctx->stats.allocations == 2);
    assert(ctx->stats.frees == 2);
    assert(ctx->stats.bytes_allocated == 96);
    assert(ctx->stats.bytes_freed == 96);

    cmalloc_memory_source_destroy(&source);

    printf("[PASS] stats accumulate\n");
}

internal void test_large_mmap(void) {
    MemorySource source = mmap_memory_source_create();

    void* ptr = memory_source_reserve(
        &source,
        MB(4),
        PTR_ALIGNMENT,
        0
    );

    assert(ptr != nullptr);

    memory_source_release(
        &source,
        ptr,
        MB(4)
    );

    mmap_memory_source_destroy(&source);

    printf("[PASS] large mmap\n");
}

int main(void) {
    test_malloc_create();
    test_cmalloc_create();
    test_mmap_create();

    test_malloc_alloc_free();
    test_cmalloc_alloc_free();
    test_mmap_alloc_free();

    test_malloc_alignment();
    test_cmalloc_alignment();

    test_multiple_allocations();
    test_null_release();
    test_stats_accumulate();

    test_large_mmap();

    printf("\nAll memory source tests passed.\n");

    return 0;
}
