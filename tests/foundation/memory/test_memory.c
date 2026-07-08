#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>
#include <base/foundation/memory/memory.h>

// ============================================================
// HELPERS
// ============================================================

internal MemorySource make_malloc_source(void) {
    return malloc_memory_source_create();
}

internal MemorySource make_cmalloc_source(void) {
    return cmalloc_memory_source_create();
}

internal MemorySource make_mmap_source(void) {
    return mmap_memory_source_create();
}

// ============================================================
// CREATE / DESTROY
// ============================================================

TEST(test_malloc_create) {
    MemorySource source = make_malloc_source();

    ASSERT_NE_PTR(source.ctx, nullptr);
    ASSERT_NE_PTR(source.vt, nullptr);

    malloc_memory_source_destroy(&source);

    ASSERT_EQ_PTR(source.ctx, nullptr);
    ASSERT_EQ_PTR(source.vt, nullptr);
}

TEST(test_cmalloc_create) {
    MemorySource source = make_cmalloc_source();

    ASSERT_NE_PTR(source.ctx, nullptr);
    ASSERT_NE_PTR(source.vt, nullptr);

    cmalloc_memory_source_destroy(&source);

    ASSERT_EQ_PTR(source.ctx, nullptr);
    ASSERT_EQ_PTR(source.vt, nullptr);
}

TEST(test_mmap_create) {
    MemorySource source = make_mmap_source();

    ASSERT_NE_PTR(source.ctx, nullptr);
    ASSERT_NE_PTR(source.vt, nullptr);

    mmap_memory_source_destroy(&source);

    ASSERT_EQ_PTR(source.ctx, nullptr);
    ASSERT_EQ_PTR(source.vt, nullptr);
}

// ============================================================
// MALLOC BACKEND
// ============================================================

TEST(test_malloc_alloc_free) {
    MemorySource source = make_malloc_source();

    void* ptr = memory_source_reserve(&source, 256, MAX_ALIGNMENT, 0);

    ASSERT_NE_PTR(ptr, nullptr);

    memory_source_release(&source, ptr, 256);

    MallocCtx* ctx = source.ctx;

    ASSERT_EQ(ctx->stats.allocations, 1);
    ASSERT_EQ(ctx->stats.frees, 1);
    ASSERT_EQ(ctx->stats.bytes_allocated, 256);
    ASSERT_EQ(ctx->stats.bytes_freed, 256);

    malloc_memory_source_destroy(&source);
}

TEST(test_malloc_alignment) {
    MemorySource source = make_malloc_source();

    void* p8  = memory_source_reserve(&source, 32, 8, 0);
    void* p16 = memory_source_reserve(&source, 32, 16, 0);
    void* p32 = memory_source_reserve(&source, 32, 32, 0);
    void* p64 = memory_source_reserve(&source, 32, 64, 0);

    ASSERT_NE_PTR(p8, nullptr);
    ASSERT_NE_PTR(p16, nullptr);
    ASSERT_NE_PTR(p32, nullptr);
    ASSERT_NE_PTR(p64, nullptr);

    ASSERT_EQ((uptr)p8  % 8, 0);
    ASSERT_EQ((uptr)p16 % 16, 0);
    ASSERT_EQ((uptr)p32 % 32, 0);
    ASSERT_EQ((uptr)p64 % 64, 0);

    memory_source_release(&source, p8, 32);
    memory_source_release(&source, p16, 32);
    memory_source_release(&source, p32, 32);
    memory_source_release(&source, p64, 64);

    malloc_memory_source_destroy(&source);
}

TEST(test_multiple_allocations) {
    MemorySource source = make_malloc_source();

    void* a = memory_source_reserve(&source, 64, 8, 0);
    void* b = memory_source_reserve(&source, 64, 8, 0);
    void* c = memory_source_reserve(&source, 64, 8, 0);

    ASSERT_NE_PTR(a, nullptr);
    ASSERT_NE_PTR(b, nullptr);
    ASSERT_NE_PTR(c, nullptr);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(b != c);

    memory_source_release(&source, a, 64);
    memory_source_release(&source, b, 64);
    memory_source_release(&source, c, 64);

    MallocCtx* ctx = source.ctx;

    ASSERT_EQ(ctx->stats.allocations, 3);
    ASSERT_EQ(ctx->stats.frees, 3);

    malloc_memory_source_destroy(&source);
}

TEST(test_null_release) {
    MemorySource source = make_malloc_source();

    memory_source_release(&source, nullptr, 64);

    MallocCtx* ctx = source.ctx;

    ASSERT_EQ(ctx->stats.allocations, 0);
    ASSERT_EQ(ctx->stats.frees, 0);

    malloc_memory_source_destroy(&source);
}

// ============================================================
// CMALLOC BACKEND
// ============================================================

TEST(test_cmalloc_alloc_free) {
    MemorySource source = make_cmalloc_source();

    void* ptr = memory_source_reserve(&source, 256, 64, 0);

    ASSERT_NE_PTR(ptr, nullptr);
    ASSERT_EQ((uptr)ptr % 64, 0);

    memory_source_release(&source, ptr, 256);

    CmallocCtx* ctx = source.ctx;

    ASSERT_EQ(ctx->stats.allocations, 1);
    ASSERT_EQ(ctx->stats.frees, 1);
    ASSERT_EQ(ctx->stats.bytes_allocated, 256);
    ASSERT_EQ(ctx->stats.bytes_freed, 256);

    cmalloc_memory_source_destroy(&source);
}

TEST(test_cmalloc_alignment) {
    MemorySource source = make_cmalloc_source();

    void* p8   = memory_source_reserve(&source, 32, 8, 0);
    void* p16  = memory_source_reserve(&source, 32, 16, 0);
    void* p32  = memory_source_reserve(&source, 32, 32, 0);
    void* p64  = memory_source_reserve(&source, 32, 64, 0);
    void* p128 = memory_source_reserve(&source, 32, 128, 0);

    ASSERT_EQ((uptr)p8   % MAX_ALIGNMENT, 0);
    ASSERT_EQ((uptr)p16  % 16, 0);
    ASSERT_EQ((uptr)p32  % 32, 0);
    ASSERT_EQ((uptr)p64  % 64, 0);
    ASSERT_EQ((uptr)p128 % 128, 0);

    memory_source_release(&source, p8, 32);
    memory_source_release(&source, p16, 32);
    memory_source_release(&source, p32, 32);
    memory_source_release(&source, p64, 32);
    memory_source_release(&source, p128, 32);

    cmalloc_memory_source_destroy(&source);
}

TEST(test_stats_accumulate) {
    MemorySource source = make_cmalloc_source();

    void* a = memory_source_reserve(&source, 32, 16, 0);
    void* b = memory_source_reserve(&source, 64, 32, 0);

    memory_source_release(&source, a, 32);
    memory_source_release(&source, b, 64);

    CmallocCtx* ctx = source.ctx;

    ASSERT_EQ(ctx->stats.allocations, 2);
    ASSERT_EQ(ctx->stats.frees, 2);
    ASSERT_EQ(ctx->stats.bytes_allocated, 96);
    ASSERT_EQ(ctx->stats.bytes_freed, 96);

    cmalloc_memory_source_destroy(&source);
}

// ============================================================
// MMAP BACKEND
// ============================================================

TEST(test_mmap_alloc_free) {
    MemorySource source = make_mmap_source();

    void* ptr = memory_source_reserve(&source, KB(16), MAX_ALIGNMENT, 0);

    ASSERT_NE_PTR(ptr, nullptr);

    memory_source_release(&source, ptr, KB(16));

    MmapCtx* ctx = source.ctx;

    ASSERT_EQ(ctx->stats.allocations, 1);
    ASSERT_EQ(ctx->stats.frees, 1);
    ASSERT_EQ(ctx->stats.bytes_allocated, KB(16));
    ASSERT_EQ(ctx->stats.bytes_freed, KB(16));

    mmap_memory_source_destroy(&source);
}

TEST(test_large_mmap) {
    MemorySource source = make_mmap_source();

    void* ptr = memory_source_reserve(&source, MB(4), MAX_ALIGNMENT, 0);

    ASSERT_NE_PTR(ptr, nullptr);

    memory_source_release(&source, ptr, MB(4));

    mmap_memory_source_destroy(&source);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(MEMORY_SOURCE, "Memory Source Tests",
    NULL,
    NULL,

    TEST_GROUP("Creation",
        TEST_NODE(test_malloc_create),
        TEST_NODE(test_cmalloc_create),
        TEST_NODE(test_mmap_create)
    ),

    TEST_GROUP("Malloc",
        TEST_NODE(test_malloc_alloc_free),
        TEST_NODE(test_malloc_alignment),
        TEST_NODE(test_multiple_allocations),
        TEST_NODE(test_null_release)
    ),

    TEST_GROUP("Cmalloc",
        TEST_NODE(test_cmalloc_alloc_free),
        TEST_NODE(test_cmalloc_alignment),
        TEST_NODE(test_stats_accumulate)
    ),

    TEST_GROUP("Mmap",
        TEST_NODE(test_mmap_alloc_free),
        TEST_NODE(test_large_mmap)
    )
);

TEST_PROGRAM();
