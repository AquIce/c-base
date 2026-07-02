#define TEST_FAIL_FATAL

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
// CREATION BOUNDARIES
// ============================================================

TEST(test_zero_capacity) { }
TEST(test_one_capacity) { }

// ============================================================
// RESERVE BOUNDARIES
// ============================================================

TEST(test_reserve_same_capacity) { }
TEST(test_reserve_smaller_capacity) { }
TEST(test_reserve_larger_capacity) { }

// ============================================================
// RESIZE BOUNDARIES
// ============================================================

TEST(test_resize_same_size) { }
TEST(test_resize_zero) { }
TEST(test_resize_beyond_capacity) { }

// ============================================================
// PUSH / POP BOUNDARIES
// ============================================================

TEST(test_push_into_empty) { }
TEST(test_push_until_growth) { }
TEST(test_pop_last_element) { }
TEST(test_full_empty_full_cycle) { }

// ============================================================
// INSERT / REMOVE BOUNDARIES
// ============================================================

TEST(test_insert_front) { }
TEST(test_insert_back) { }
TEST(test_remove_front) { }
TEST(test_remove_back) { }

// ============================================================
// APPEND BOUNDARIES
// ============================================================

TEST(test_append_zero_elements) { }
TEST(test_append_exact_capacity) { }
TEST(test_append_requires_growth) { }

// ============================================================
// LIFETIME / STATE SAFETY
// ============================================================

TEST(test_clear_twice) { }
TEST(test_reset_twice) { }
TEST(test_double_destroy) { }

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

    TEST_GROUP("Idempotence & Lifetime",
        TEST_NODE(test_clear_twice),
        TEST_NODE(test_reset_twice),
        TEST_NODE(test_double_destroy)
    )
);

TEST_PROGRAM();
