#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>

#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>

typedef struct {
    i32 x;
    i32 y;
    f32 z;
} TestStruct;

// ============================================================
// FIXTURE
// ============================================================

internal MemorySource source;
internal Allocator arena;

internal void setup(void) {
    source = malloc_memory_source_create();
    arena = arena_create(&source, KB(1));
}

internal void teardown(void) {
    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);
}

// ============================================================
// CREATION
// ============================================================

TEST(test_create) {
    ASSERT_NE_PTR(arena.handler, nullptr);
    ASSERT_NE_PTR(arena.vt, nullptr);
    ASSERT_EQ_PTR(arena.source, &source);

    arena_destroy(&arena);

    ASSERT_EQ_PTR(arena.handler, nullptr);
    ASSERT_EQ_PTR(arena.vt, nullptr);
    ASSERT_EQ_PTR(arena.source, nullptr);

    arena = arena_create(&source, KB(1));
}

// ============================================================
// ALLOCATION
// ============================================================

TEST(test_alloc_single) {
    i32* value = ALLOC(&arena, i32);

    ASSERT_NE_PTR(value, nullptr);

    *value = 42;

    ASSERT_EQ(*value, 42);
}

TEST(test_alloc_array) {
    i32* values = NALLOC(&arena, i32, 128);

    ASSERT_NE_PTR(values, nullptr);

    for(usize i = 0; i < 128; i++) {
        values[i] = (i32)i;
	}

    for(usize i = 0; i < 128; i++) {
        ASSERT_EQ(values[i], (i32)i);
	}
}

TEST(test_multiple_allocs) {
    TestStruct* a = ALLOC(&arena, TestStruct);
    TestStruct* b = ALLOC(&arena, TestStruct);
    TestStruct* c = ALLOC(&arena, TestStruct);

    ASSERT_NE_PTR(a, nullptr);
    ASSERT_NE_PTR(b, nullptr);
    ASSERT_NE_PTR(c, nullptr);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(b != c);
}

// ============================================================
// ALIGNMENT
// ============================================================

TEST(test_alignment) {
    void* p8  = allocator_alloc(&arena, 16, 8);
    void* p16 = allocator_alloc(&arena, 16, 16);
    void* p32 = allocator_alloc(&arena, 16, 32);
    void* p64 = allocator_alloc(&arena, 16, 64);

    ASSERT_NE_PTR(p8, nullptr);
    ASSERT_NE_PTR(p16, nullptr);
    ASSERT_NE_PTR(p32, nullptr);
    ASSERT_NE_PTR(p64, nullptr);

    ASSERT_EQ((uptr)p8  % 8, 0);
    ASSERT_EQ((uptr)p16 % 16, 0);
    ASSERT_EQ((uptr)p32 % 32, 0);
    ASSERT_EQ((uptr)p64 % 64, 0);
}

// ============================================================
// CAPACITY
// ============================================================

TEST(test_out_of_memory) {
    Allocator local_arena = arena_create(&source, 128);

    void* first  = allocator_alloc(&local_arena, 100, 8);
    void* second = allocator_alloc(&local_arena, 100, 8);

    ASSERT_NE_PTR(first, nullptr);
    ASSERT_EQ_PTR(second, nullptr);

    arena_destroy(&local_arena);
}

TEST(test_exact_capacity) {
    Allocator local_arena = arena_create(&source, 128);

    ASSERT_NE_PTR(allocator_alloc(&local_arena, 128, 1), nullptr);
    ASSERT_EQ_PTR(allocator_alloc(&local_arena, 1, 1), nullptr);

    arena_destroy(&local_arena);
}

// ============================================================
// RESET
// ============================================================

TEST(test_reset) {
	allocator_reset(&arena);
    void* first = allocator_alloc(&arena, 128, 8);

    allocator_reset(&arena);

    void* second = allocator_alloc(&arena, 128, 8);

    ASSERT_EQ_PTR(first, second);
}

// ============================================================
// API
// ============================================================

TEST(test_free_noop) {
    i32* first = ALLOC(&arena, i32);

    allocator_free(&arena, first);

    i32* second = ALLOC(&arena, i32);

    ASSERT_NE_PTR(second, nullptr);
    ASSERT_TRUE(second != first);
}

TEST(test_realloc_stub) {
    i32* value = ALLOC(&arena, i32);

    ASSERT_EQ_PTR(
        allocator_realloc(&arena, value, sizeof(i32), sizeof(i64)),
        nullptr
    );
}

// ============================================================
// EXTERNAL BUFFER
// ============================================================

TEST(test_external_buffer) {
    u8 buffer[512];

    Allocator local_arena = arena_create_from_buffer(&source, buffer, sizeof(buffer));

    TestStruct* value = ALLOC(&local_arena, TestStruct);

    ASSERT_NE_PTR(value, nullptr);

    ASSERT_TRUE((u8*)value >= buffer);
    ASSERT_TRUE((u8*)value < buffer + sizeof(buffer));

    arena_destroy(&local_arena);
}

TEST(test_external_buffer_reset) {
    u8 buffer[256];

    Allocator local_arena = arena_create_from_buffer(&source, buffer, sizeof(buffer));

    void* first = allocator_alloc(&local_arena, 64, 8);

    allocator_reset(&local_arena);

    void* second = allocator_alloc(&local_arena, 64, 8);

    ASSERT_EQ_PTR(first, second);

    arena_destroy(&local_arena);
}

// ============================================================
// MISC
// ============================================================

TEST(test_source_association) {
    ASSERT_EQ_PTR(arena.source, &source);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(ARENA, "Arena Tests",
    setup,
    teardown,

    TEST_GROUP("Creation",
        TEST_NODE(test_create)
    ),

    TEST_GROUP("Allocation",
        TEST_NODE(test_alloc_single),
        TEST_NODE(test_alloc_array),
        TEST_NODE(test_multiple_allocs)
    ),

    TEST_GROUP("Alignment",
        TEST_NODE(test_alignment)
    ),

    TEST_GROUP("Capacity",
        TEST_NODE(test_out_of_memory),
        TEST_NODE(test_exact_capacity)
    ),

    TEST_GROUP("Reset",
        TEST_NODE(test_reset)
    ),

    TEST_GROUP("API",
        TEST_NODE(test_free_noop),
        TEST_NODE(test_realloc_stub)
    ),

    TEST_GROUP("External Buffer",
        TEST_NODE(test_external_buffer),
        TEST_NODE(test_external_buffer_reset)
    ),

    TEST_GROUP("Misc",
        TEST_NODE(test_source_association)
    )
);

TEST_PROGRAM();
