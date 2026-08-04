
#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>
#include <base/foundation/containers/container.h>
#include <base/foundation/containers/hashmap.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>

// ============================================================
// GLOBAL TEST FIXTURE (ROOT-OWNED)
// ============================================================

internal MemorySource mem_source;
internal Allocator arena;

internal void setup_arena(void) {
    mem_source = malloc_memory_source_create();
    arena = arena_create(&mem_source, 4096);
}

internal void teardown_arena(void) {
    arena_destroy(&arena);
}

// ============================================================
// POLICY CALLBACKS
// ============================================================

internal bool test_equals(void* ctx, const void* elem, const void* other) {
	return *(usize*)elem == *(usize*)other;
}

internal hash_t test_hash(void* ctx, const void* object) {
	return *(usize*)object;
}

internal const ElementPolicy TEST_POLICY = {
	.equals = &test_equals,
	.hash = &test_hash,
};

internal const ElementLifetime key_lifetime = {
	.ctx = nullptr,
	.policy = &TEST_POLICY,
};

// ============================================================
// POD TYPES
// ============================================================

typedef struct {
    i32 x;
    i32 y;
} TestPOD;

// ============================================================
// HELPERS
// ============================================================

internal HashMap make_hashmap(usize cap) {
    return hashmap_create(
		&arena,
		cap,
		sizeof(usize),
		sizeof(TestPOD),
		alignof(usize),
		alignof(TestPOD),
		HASHMAP_HASH_STRATEGRY_RECOMPUTE,
		&key_lifetime
	);
}

// ============================================================
// CREATION BOUNDARIES
// ============================================================

TEST(test_zero_capacity) {
    HashMap hashmap = make_hashmap(0);

    ASSERT_EQ(hashmap_size(&hashmap), 0);
    ASSERT_EQ(hashmap_capacity(&hashmap), 0);
    ASSERT_EQ_PTR(hashmap.buffer, nullptr);
    ASSERT_EQ_PTR(hashmap.keys_buffer, nullptr);
    ASSERT_EQ_PTR(hashmap.meta_buffer, nullptr);

    hashmap_destroy(&hashmap);
}

TEST(test_one_capacity) {
    HashMap hashmap = make_hashmap(1);

    ASSERT_EQ(hashmap_size(&hashmap), 0);
    ASSERT_EQ(hashmap_capacity(&hashmap), 1);
    ASSERT_TRUE(hashmap.buffer != nullptr);
    ASSERT_TRUE(hashmap.keys_buffer != nullptr);
    ASSERT_TRUE(hashmap.meta_buffer != nullptr);

    hashmap_destroy(&hashmap);
}

// ============================================================
// GROW BOUNDARIES
// ============================================================

TEST(test_grow_same_capacity) {
    HashMap hashmap = make_hashmap(4);

    void* old = hashmap.buffer;

    ASSERT_TRUE(hashmap_grow(&hashmap, 4));
    ASSERT_EQ_PTR(hashmap.buffer, old);

    hashmap_destroy(&hashmap);
}

TEST(test_grow_smaller_capacity) {
    HashMap hashmap = make_hashmap(4);

    ASSERT_FALSE(
		hashmap_grow(&hashmap, 2)
	);

    hashmap_destroy(&hashmap);
}

TEST(test_grow_larger_capacity) {
    HashMap hashmap = make_hashmap(2);

    ASSERT_TRUE(hashmap_grow(&hashmap, 16));
    ASSERT_EQ(hashmap_capacity(&hashmap), 16);
    ASSERT_TRUE(hashmap.buffer != nullptr);

    hashmap_destroy(&hashmap);
}

// ============================================================
// PUSH / POP BOUNDARIES
// ============================================================

TEST(test_push_into_empty) {
    HashMap hashmap = make_hashmap(0);

	usize key = 42;
	TestPOD elem = (TestPOD){
		.x = 1,
		.y = 1,
	};
    ASSERT_TRUE(hashmap_insert(&hashmap, &key, &elem));

    ASSERT_EQ(hashmap_size(&hashmap), 1);

    hashmap_destroy(&hashmap);
}

TEST(test_push_until_growth) {
    HashMap hashmap = make_hashmap(1);

	usize k1 = 1, k2 = 7;
	TestPOD
		e1 = { .x = 1, .y = 1 },
		e2 = { .x = 2, .y = 2 };

    ASSERT_TRUE(hashmap_insert(&hashmap, &k1, &e1));
    ASSERT_TRUE(hashmap_insert(&hashmap, &k2, &e2));

    ASSERT_EQ(hashmap_size(&hashmap), 2);
    ASSERT_EQ(hashmap_capacity(&hashmap), 2);

    hashmap_destroy(&hashmap);
}

// ============================================================
// LIFETIME / STATE SAFETY
// ============================================================

TEST(test_clear_twice) {
    HashMap hashmap = make_hashmap(2);

	usize key = 7;
	TestPOD elem = (TestPOD){
		.x = 1,
		.y = 1,
	};
    hashmap_insert(&hashmap, &key, &elem);

    hashmap_clear(&hashmap);
    hashmap_clear(&hashmap); // must not crash

    ASSERT_EQ(hashmap_size(&hashmap), 0);

    hashmap_destroy(&hashmap);
}

TEST(test_double_destroy) {
    HashMap hashmap = make_hashmap(2);

    hashmap_destroy(&hashmap);
    hashmap_destroy(&hashmap); // should be safe

    ASSERT_TRUE(true);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(HASHMAP_EDGES, "HashMap Edge Tests",
    setup_arena,
    teardown_arena,

    TEST_GROUP("Creation",
        TEST_NODE(test_zero_capacity),
        TEST_NODE(test_one_capacity)
    ),

    TEST_GROUP("Grow",
        TEST_NODE(test_grow_same_capacity),
        TEST_NODE(test_grow_smaller_capacity),
        TEST_NODE(test_grow_larger_capacity)
    ),

    TEST_GROUP("Push / Pop",
        TEST_NODE(test_push_into_empty),
        TEST_NODE(test_push_until_growth)
    ),

    TEST_GROUP("Lifetime / State",
        TEST_NODE(test_clear_twice),
        TEST_NODE(test_double_destroy)
    )
);

TEST_PROGRAM();
