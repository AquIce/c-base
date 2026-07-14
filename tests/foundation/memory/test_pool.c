#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>

#include <base/foundation/memory/pool.h>
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
internal Allocator pool;

internal void setup(void) {
    source = malloc_memory_source_create();
    pool = POOL_CREATE_T(&source, TestStruct, 16);
}

internal void teardown(void) {
    pool_destroy(&pool);
    malloc_memory_source_destroy(&source);
}

internal void node_setup(void) {
	allocator_reset(&pool);
}

// ============================================================
// CREATION
// ============================================================

TEST(test_create) {
    ASSERT_NE_PTR(pool.handler, nullptr);
    ASSERT_NE_PTR(pool.vt, nullptr);
    ASSERT_EQ_PTR(pool.source, &source);

    pool_destroy(&pool);

    ASSERT_EQ_PTR(pool.handler, nullptr);
    ASSERT_EQ_PTR(pool.vt, nullptr);
    ASSERT_EQ_PTR(pool.source, nullptr);

    pool = POOL_CREATE_T(&source, TestStruct, 16);
}

// ============================================================
// ALLOCATION
// ============================================================

TEST(test_alloc_single) {
    TestStruct* value = ALLOC(&pool, TestStruct);

    ASSERT_NE_PTR(value, nullptr);

    value->x = 10;
    value->y = 20;
    value->z = 30.0f;

    ASSERT_EQ(value->x, 10);
    ASSERT_EQ(value->y, 20);
    ASSERT_EQ(value->z, 30.0f);
}

TEST(test_multiple_allocs) {
    TestStruct* a = ALLOC(&pool, TestStruct);
    TestStruct* b = ALLOC(&pool, TestStruct);
    TestStruct* c = ALLOC(&pool, TestStruct);

    ASSERT_NE_PTR(a, nullptr);
    ASSERT_NE_PTR(b, nullptr);
    ASSERT_NE_PTR(c, nullptr);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(b != c);
}

// ============================================================
// CAPACITY
// ============================================================

TEST(test_out_of_blocks) {
    Allocator local = POOL_CREATE_T(&source, TestStruct, 2);

    ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);
    ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);

    ASSERT_EQ_PTR(ALLOC(&local, TestStruct), nullptr);

    pool_destroy(&local);
}

TEST(test_exact_capacity) {
    Allocator local = POOL_CREATE_T(&source, TestStruct, 4);

    ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);
    ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);
    ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);
    ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);

    ASSERT_EQ_PTR(ALLOC(&local, TestStruct), nullptr);

    pool_destroy(&local);
}

// ============================================================
// FREE
// ============================================================

TEST(test_free_reuses_block) {
    TestStruct* first = ALLOC(&pool, TestStruct);

    allocator_free(&pool, first);

    TestStruct* second = ALLOC(&pool, TestStruct);

    ASSERT_EQ_PTR(first, second);
}

TEST(test_free_lifo) {
    TestStruct* a = ALLOC(&pool, TestStruct);
    TestStruct* b = ALLOC(&pool, TestStruct);
    TestStruct* c = ALLOC(&pool, TestStruct);

    allocator_free(&pool, a);
    allocator_free(&pool, b);
    allocator_free(&pool, c);

    ASSERT_EQ_PTR(ALLOC(&pool, TestStruct), c);
    ASSERT_EQ_PTR(ALLOC(&pool, TestStruct), b);
    ASSERT_EQ_PTR(ALLOC(&pool, TestStruct), a);
}

// ============================================================
// RESET
// ============================================================

TEST(test_reset) {
    TestStruct* first = ALLOC(&pool, TestStruct);

    allocator_reset(&pool);

    TestStruct* second = ALLOC(&pool, TestStruct);

    ASSERT_EQ_PTR(first, second);
}

TEST(test_reset_restores_capacity) {
    for(usize i = 0; i < 16; i++) {
        ASSERT_NE_PTR(ALLOC(&pool, TestStruct), nullptr);
	}

    ASSERT_EQ_PTR(ALLOC(&pool, TestStruct), nullptr);

    allocator_reset(&pool);

    for(usize i = 0; i < 16; i++) {
        ASSERT_NE_PTR(ALLOC(&pool, TestStruct), nullptr);
	}

    ASSERT_EQ_PTR(ALLOC(&pool, TestStruct), nullptr);
}

// ============================================================
// API
// ============================================================

TEST(test_realloc_stub) {
    TestStruct* value = ALLOC(&pool, TestStruct);

    ASSERT_EQ_PTR(
        allocator_realloc(
            &pool,
            value,
            sizeof(TestStruct),
            sizeof(TestStruct)
        ),
        nullptr
    );
}

// ============================================================
// STRESS
// ============================================================

TEST(test_allocate_free_allocate_all) {
    Allocator local = POOL_CREATE_T(&source, TestStruct, 8);

    TestStruct* ptrs[8];

    for(usize i = 0; i < 8; i++) {
        ptrs[i] = ALLOC(&local, TestStruct);
	}

    for(usize i = 0; i < 8; i++) {
        allocator_free(&local, ptrs[i]);
	}

    for(usize i = 0; i < 8; i++) {
        ASSERT_NE_PTR(ALLOC(&local, TestStruct), nullptr);
	}

    ASSERT_EQ_PTR(ALLOC(&local, TestStruct), nullptr);

    pool_destroy(&local);
}

// ============================================================
// MISC
// ============================================================

TEST(test_source_association) {
    ASSERT_EQ_PTR(pool.source, &source);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(POOL, "Pool Tests",
    setup,
    teardown,

    TEST_GROUP("Creation",
        TEST_NODE(test_create)
    ),

    TEST_GROUP("Allocation",
        TEST_NODE(test_alloc_single),
        TEST_NODE(test_multiple_allocs)
    ),

    TEST_GROUP("Capacity",
        TEST_NODE(test_out_of_blocks),
        TEST_NODE(test_exact_capacity)
    ),

    TEST_GROUP("Free",
        TEST_NODE(test_free_reuses_block),
        TEST_NODE(test_free_lifo)
    ),

    TEST_GROUP("Reset",
        TEST_NODE_SETUP(test_reset, node_setup),
        TEST_NODE_SETUP(test_reset_restores_capacity, node_setup)
    ),

    TEST_GROUP("API",
        TEST_NODE(test_realloc_stub)
    ),

    TEST_GROUP("Stress",
        TEST_NODE(test_allocate_free_allocate_all)
    ),

    TEST_GROUP("Misc",
        TEST_NODE(test_source_association)
    )
);

TEST_PROGRAM();
