#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>
#include <base/foundation/containers/dynarray.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>

// ============================================================
// FIXTURE
// ============================================================

internal MemorySource mem_source;
internal Allocator arena;

internal void setup_arena(void) {
    mem_source = malloc_memory_source_create();
    arena = arena_create(&mem_source, KB(4));
}

internal void teardown_arena(void) {
    arena_destroy(&arena);
}

// ============================================================
// HELPERS
// ============================================================

internal DynArray make_array(usize cap) {
    return dynarray_create(&arena, cap, sizeof(i32), alignof(i32));
}

// ============================================================
// CREATION BOUNDARIES
// ============================================================

TEST(test_zero_capacity) {
    DynArray a = make_array(0);

    ASSERT_EQ(dynarray_size(&a), 0);
    ASSERT_EQ(dynarray_capacity(&a), 0);
    ASSERT_EQ_PTR(a.buffer, nullptr);

    dynarray_destroy(&a);
}

TEST(test_one_capacity) {
    DynArray a = make_array(1);

    ASSERT_EQ(dynarray_size(&a), 0);
    ASSERT_EQ(dynarray_capacity(&a), 1);
    ASSERT_TRUE(a.buffer != nullptr);

    dynarray_destroy(&a);
}

// ============================================================
// RESERVE BOUNDARIES
// ============================================================

TEST(test_reserve_same_capacity) {
    DynArray a = make_array(4);

    void* old = a.buffer;

    ASSERT_TRUE(dynarray_reserve(&a, 4));
    ASSERT_EQ_PTR(a.buffer, old);

    dynarray_destroy(&a);
}

TEST(test_reserve_smaller_capacity) {
    DynArray a = make_array(4);

    void* old = a.buffer;

    ASSERT_TRUE(dynarray_reserve(&a, 2));
    ASSERT_EQ_PTR(a.buffer, old);

    dynarray_destroy(&a);
}

TEST(test_reserve_larger_capacity) {
    DynArray a = make_array(2);

    ASSERT_TRUE(dynarray_reserve(&a, 16));
    ASSERT_EQ(dynarray_capacity(&a), 16);
    ASSERT_TRUE(a.buffer != nullptr);

    dynarray_destroy(&a);
}

// ============================================================
// RESIZE BOUNDARIES
// ============================================================

TEST(test_resize_same_size) {
    DynArray a = make_array(4);

    ASSERT_TRUE(dynarray_resize(&a, 0));
    ASSERT_EQ(dynarray_size(&a), 0);

    dynarray_destroy(&a);
}

TEST(test_resize_zero) {
    DynArray a = make_array(4);

    ASSERT_TRUE(dynarray_resize(&a, 0));
    ASSERT_EQ(dynarray_size(&a), 0);

    dynarray_destroy(&a);
}

TEST(test_resize_beyond_capacity) {
    DynArray a = make_array(4);

    ASSERT_TRUE(!dynarray_resize(&a, 10));
    ASSERT_EQ(dynarray_size(&a), 0);

    dynarray_destroy(&a);
}

// ============================================================
// PUSH / POP BOUNDARIES
// ============================================================

TEST(test_push_into_empty) {
    DynArray a = make_array(0);

    i32 v = 42;
    ASSERT_TRUE(dynarray_push(&a, &v));

    ASSERT_EQ(dynarray_size(&a), 1);
    ASSERT_EQ(*(i32*)a.buffer, 42);

    dynarray_destroy(&a);
}

TEST(test_push_until_growth) {
    DynArray a = make_array(1);

    i32 v1 = 1, v2 = 2;

    ASSERT_TRUE(dynarray_push(&a, &v1));
    ASSERT_TRUE(dynarray_push(&a, &v2));

    ASSERT_EQ(dynarray_size(&a), 2);
    ASSERT_EQ(dynarray_capacity(&a), 2); // growth factor assumed >= 2

    dynarray_destroy(&a);
}

TEST(test_pop_last_element) {
    DynArray a = make_array(2);

    i32 v = 1;
    dynarray_push(&a, &v);

    dynarray_pop(&a);

    ASSERT_EQ(dynarray_size(&a), 0);

    dynarray_destroy(&a);
}

TEST(test_full_empty_full_cycle) {
    DynArray a = make_array(1);

    i32 v1 = 1, v2 = 2;

    dynarray_push(&a, &v1);
    dynarray_pop(&a);
    dynarray_push(&a, &v2);

    ASSERT_EQ(dynarray_size(&a), 1);
    ASSERT_EQ(*(i32*)a.buffer, 2);

    dynarray_destroy(&a);
}

// ============================================================
// INSERT / REMOVE BOUNDARIES
// ============================================================

TEST(test_insert_front) {
    DynArray a = make_array(2);

    i32 a1 = 1, a2 = 2;
    dynarray_push(&a, &a2);

    ASSERT_TRUE(dynarray_insert(&a, 0, &a1));

    ASSERT_EQ(*(i32*)a.buffer, 1);

    dynarray_destroy(&a);
}

TEST(test_insert_back) {
    DynArray a = make_array(2);

    i32 a1 = 1, a2 = 2;

    dynarray_push(&a, &a1);
    dynarray_insert(&a, 1, &a2);

    ASSERT_EQ(*(i32*)((char*)a.buffer + sizeof(i32)), 2);

    dynarray_destroy(&a);
}

TEST(test_remove_front) {
    DynArray a = make_array(2);

    i32 v1 = 1, v2 = 2;

    dynarray_push(&a, &v1);
    dynarray_push(&a, &v2);

    dynarray_remove(&a, 0);

    ASSERT_EQ(*(i32*)a.buffer, 2);
    ASSERT_EQ(dynarray_size(&a), 1);

    dynarray_destroy(&a);
}

TEST(test_remove_back) {
    DynArray a = make_array(2);

    i32 v1 = 1, v2 = 2;

    dynarray_push(&a, &v1);
    dynarray_push(&a, &v2);

    dynarray_remove(&a, 1);

    ASSERT_EQ(dynarray_size(&a), 1);

    dynarray_destroy(&a);
}

// ============================================================
// APPEND BOUNDARIES
// ============================================================

TEST(test_append_zero_elements) {
    DynArray a = make_array(4);

    i32 arr[] = {1,2,3};

    ASSERT_TRUE(dynarray_append(&a, arr, 0));
    ASSERT_EQ(dynarray_size(&a), 0);

    dynarray_destroy(&a);
}

TEST(test_append_exact_capacity) {
    DynArray a = make_array(3);

    i32 arr[] = {1,2,3};

    ASSERT_TRUE(dynarray_append(&a, arr, 3));
    ASSERT_EQ(dynarray_size(&a), 3);

    dynarray_destroy(&a);
}

TEST(test_append_requires_growth) {
    DynArray a = make_array(1);

    i32 arr[] = {1,2,3,4};

    ASSERT_TRUE(dynarray_append(&a, arr, 4));
    ASSERT_EQ(dynarray_size(&a), 4);

    dynarray_destroy(&a);
}

// ============================================================
// LIFETIME / STATE SAFETY
// ============================================================

TEST(test_clear_twice) {
    DynArray a = make_array(2);

    i32 v = 1;
    dynarray_push(&a, &v);

    dynarray_clear(&a);
    dynarray_clear(&a); // must not crash

    ASSERT_EQ(dynarray_size(&a), 0);

    dynarray_destroy(&a);
}

TEST(test_reset_twice) {
    DynArray a = make_array(2);

    dynarray_destroy(&a);
    dynarray_destroy(&a); // must not crash

    ASSERT_TRUE(true);
}

TEST(test_double_destroy) {
    DynArray a = make_array(2);

    dynarray_destroy(&a);
    dynarray_destroy(&a); // should be safe

    ASSERT_TRUE(true);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(DYNARRAY_EDGES, "DynArray Edge Tests",
    setup_arena,
    teardown_arena,

    TEST_GROUP("Creation",
        TEST_NODE(test_zero_capacity),
        TEST_NODE(test_one_capacity)
    ),

    TEST_GROUP("Reserve",
        TEST_NODE(test_reserve_same_capacity),
        TEST_NODE(test_reserve_smaller_capacity),
        TEST_NODE(test_reserve_larger_capacity)
    ),

    TEST_GROUP("Resize",
        TEST_NODE(test_resize_same_size),
        TEST_NODE(test_resize_zero),
        TEST_NODE(test_resize_beyond_capacity)
    ),

    TEST_GROUP("Push / Pop",
        TEST_NODE(test_push_into_empty),
        TEST_NODE(test_push_until_growth),
        TEST_NODE(test_pop_last_element),
        TEST_NODE(test_full_empty_full_cycle)
    ),

    TEST_GROUP("Insert / Remove",
        TEST_NODE(test_insert_front),
        TEST_NODE(test_insert_back),
        TEST_NODE(test_remove_front),
        TEST_NODE(test_remove_back)
    ),

    TEST_GROUP("Append",
        TEST_NODE(test_append_zero_elements),
        TEST_NODE(test_append_exact_capacity),
        TEST_NODE(test_append_requires_growth)
    ),

    TEST_GROUP("Lifetime / State",
        TEST_NODE(test_clear_twice),
        TEST_NODE(test_reset_twice),
        TEST_NODE(test_double_destroy)
    )
);

TEST_PROGRAM();
