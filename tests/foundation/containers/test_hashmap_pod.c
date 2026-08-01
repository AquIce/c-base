#include "base/foundation/containers/container.h"
#include <base/foundation/core/test.h>
#include <base/foundation/macros.h>

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
// CREATION / DESTRUCTION
// ============================================================

TEST(test_pod_create) {
    HashMap hashmap = hashmap_create(
		&arena,
		4,
		sizeof(usize),
		sizeof(TestPOD),
		alignof(usize),
		alignof(TestPOD),
		HASHMAP_HASH_STRATEGRY_RECOMPUTE,
		&key_lifetime
	);

    ASSERT_TRUE(hashmap.buffer != nullptr);
    ASSERT_EQ(hashmap.elem_count, 0);

    ASSERT_EQ(hashmap.descriptor.capacity, 4);
    ASSERT_EQ(hashmap.descriptor.key_size, sizeof(usize));
    ASSERT_EQ(hashmap.descriptor.elem_size, sizeof(TestPOD));
    ASSERT_EQ(hashmap.descriptor.key_alignment, alignof(usize));
    ASSERT_EQ(hashmap.descriptor.elem_alignment, alignof(TestPOD));
    ASSERT_EQ_PTR(hashmap.descriptor.allocator, &arena);
    ASSERT_EQ_PTR(hashmap.descriptor.key_lifetime, &key_lifetime);
    ASSERT_EQ_PTR(hashmap.descriptor.elem_lifetime, nullptr);
}

TEST(test_pod_create_macro) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

    ASSERT_TRUE(hashmap.buffer != nullptr);
    ASSERT_EQ(hashmap.elem_count, 0);

    ASSERT_EQ(hashmap.descriptor.capacity, 4);
    ASSERT_EQ(hashmap.descriptor.key_size, sizeof(usize));
    ASSERT_EQ(hashmap.descriptor.elem_size, sizeof(TestPOD));
    ASSERT_EQ(hashmap.descriptor.key_alignment, alignof(usize));
    ASSERT_EQ(hashmap.descriptor.elem_alignment, alignof(TestPOD));
    ASSERT_EQ_PTR(hashmap.descriptor.allocator, &arena);
    ASSERT_EQ_PTR(hashmap.descriptor.key_lifetime, &key_lifetime);
    ASSERT_EQ_PTR(hashmap.descriptor.elem_lifetime, nullptr);
}

TEST(test_pod_destroy) {
    HashMap hashmap = hashmap_create(
		&arena,
		4,
		sizeof(usize),
		sizeof(TestPOD),
		alignof(usize),
		alignof(TestPOD),
		HASHMAP_HASH_STRATEGRY_RECOMPUTE,
		&key_lifetime
	);

    hashmap_destroy(&hashmap);

    ASSERT_EQ_PTR(hashmap.buffer, nullptr);
    ASSERT_EQ(hashmap.elem_count, 0);

    ASSERT_EQ(hashmap.descriptor.capacity, 0);
    ASSERT_EQ(hashmap.descriptor.key_size, 0);
    ASSERT_EQ(hashmap.descriptor.elem_size, 0);
    ASSERT_EQ(hashmap.descriptor.key_alignment, 0);
    ASSERT_EQ(hashmap.descriptor.elem_alignment, 0);
    ASSERT_EQ_PTR(hashmap.descriptor.allocator, nullptr);
    ASSERT_EQ_PTR(hashmap.descriptor.key_lifetime, nullptr);
    ASSERT_EQ_PTR(hashmap.descriptor.elem_lifetime, nullptr);
}

// ============================================================
// SIZE
// ============================================================

TEST(test_pod_grow) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

    ASSERT_EQ(hashmap.descriptor.capacity, 4);

	hashmap_grow(&hashmap, 16);

	ASSERT_EQ(hashmap.elem_count, 0);
	ASSERT_EQ(hashmap.descriptor.capacity, 16);
}

// ============================================================
// COPY
// ============================================================

TEST(test_pod_copy) {
    HashMap src = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3, kc = 6;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	ASSERT_TRUE(HASHMAP_INSERT(&src, ka, a));
	ASSERT_TRUE(HASHMAP_INSERT(&src, kb, b));
	ASSERT_TRUE(HASHMAP_INSERT(&src, kc, c));

    HashMap dst = {0};

    ASSERT_TRUE(hashmap_copy(&dst, &src));

    ASSERT_TRUE(dst.buffer != nullptr);
    ASSERT_TRUE(dst.buffer != src.buffer);

    ASSERT_EQ(dst.elem_count, src.elem_count);

    ASSERT_EQ(dst.descriptor.capacity, src.descriptor.capacity);
    ASSERT_EQ(dst.descriptor.key_size, src.descriptor.key_size);
    ASSERT_EQ(dst.descriptor.elem_size, src.descriptor.elem_size);
    ASSERT_EQ(dst.descriptor.key_alignment, src.descriptor.key_alignment);
    ASSERT_EQ(dst.descriptor.elem_alignment, src.descriptor.elem_alignment);
    ASSERT_EQ_PTR(dst.descriptor.allocator, src.descriptor.allocator);
    ASSERT_EQ_PTR(dst.descriptor.key_lifetime, src.descriptor.key_lifetime);
    ASSERT_EQ_PTR(dst.descriptor.elem_lifetime, src.descriptor.elem_lifetime);

    ASSERT_EQ(HASHMAP_AT(&dst, TestPOD, ka)->x, 1);
    ASSERT_EQ(HASHMAP_AT(&dst, TestPOD, kb)->x, 2);
    ASSERT_EQ(HASHMAP_AT(&dst, TestPOD, kc)->x, 3);

    hashmap_destroy(&dst);
    hashmap_destroy(&src);
}

TEST(test_pod_copy_empty) {
	HashMap src = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);
    HashMap dst = {0};

    ASSERT_TRUE(hashmap_copy(&dst, &src));

    ASSERT_TRUE(dst.buffer != nullptr);
    ASSERT_TRUE(dst.buffer != src.buffer);

    ASSERT_EQ(dst.elem_count, src.elem_count);

    ASSERT_EQ(dst.descriptor.capacity, src.descriptor.capacity);
    ASSERT_EQ(dst.descriptor.key_size, src.descriptor.key_size);
    ASSERT_EQ(dst.descriptor.elem_size, src.descriptor.elem_size);
    ASSERT_EQ(dst.descriptor.key_alignment, src.descriptor.key_alignment);
    ASSERT_EQ(dst.descriptor.elem_alignment, src.descriptor.elem_alignment);
    ASSERT_EQ_PTR(dst.descriptor.allocator, src.descriptor.allocator);
    ASSERT_EQ_PTR(dst.descriptor.key_lifetime, src.descriptor.key_lifetime);
    ASSERT_EQ_PTR(dst.descriptor.elem_lifetime, src.descriptor.elem_lifetime);

    hashmap_destroy(&dst);
    hashmap_destroy(&src);
}

// ============================================================
// ACCESS
// ============================================================

TEST(test_pod_has) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3, kc = 6;
	TestPOD
		a = { .x = 1, .y = 1 },
		c = { .x = 3, .y = 3 };

	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, ka, a));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kc, c));

	ASSERT_TRUE(hashmap_has(&hashmap, &ka));
	ASSERT_FALSE(hashmap_has(&hashmap, &kb));
	ASSERT_TRUE(hashmap_has(&hashmap, &kc));
}

TEST(test_pod_at) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	HASHMAP_INSERT(&hashmap, ka, a);
	HASHMAP_INSERT(&hashmap, kb, b);
	HASHMAP_INSERT(&hashmap, ka, c);

	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, ka)->x, 3);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kb)->x, 2);

    hashmap_destroy(&hashmap);
}

// ============================================================
// MODIFIERS
// ============================================================

TEST(test_pod_insert) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3, kc = 18;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, ka, a));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kb, b));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kc, c));

    ASSERT_EQ(hashmap_size(&hashmap), 3);

	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, ka)->x, 1);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kb)->x, 2);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kc)->x, 3);

    hashmap_destroy(&hashmap);
}

TEST(test_pod_insert_collision) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 5, kc = 9;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, ka, a));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kb, b));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kc, c));

    ASSERT_EQ(hashmap_size(&hashmap), 3);

	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, ka)->x, 1);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kb)->x, 2);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kc)->x, 3);

    hashmap_destroy(&hashmap);
}

TEST(test_pod_insert_grow) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 2, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3, kc = 18;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, ka, a));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kb, b));
	ASSERT_TRUE(HASHMAP_INSERT(&hashmap, kc, c));

    ASSERT_EQ(hashmap.descriptor.capacity, 2 * HASHMAP_GROW_FACTOR);

	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, ka)->x, 1);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kb)->x, 2);
	ASSERT_EQ(HASHMAP_AT(&hashmap, TestPOD, kc)->x, 3);

    hashmap_destroy(&hashmap);
}

TEST(test_pod_remove) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3, kc = 18;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	HASHMAP_INSERT(&hashmap, ka, a);
	HASHMAP_INSERT(&hashmap, kb, b);
	HASHMAP_INSERT(&hashmap, kc, c);

    ASSERT_EQ(hashmap_size(&hashmap), 3);

	ASSERT_TRUE(hashmap_remove(&hashmap, &ka));
	ASSERT_TRUE(hashmap_remove(&hashmap, &kc));

	ASSERT_FALSE(hashmap_has(&hashmap, &ka));
	ASSERT_TRUE(hashmap_has(&hashmap, &kb));
	ASSERT_FALSE(hashmap_has(&hashmap, &kc));

    hashmap_destroy(&hashmap);
}

TEST(test_pod_clear) {
    HashMap hashmap = HASHMAP_CREATE(usize, TestPOD, &arena, 4, HASHMAP_HASH_STRATEGRY_RECOMPUTE, &key_lifetime);

	usize ka = 1, kb = 3, kc = 18;
	TestPOD
		a = { .x = 1, .y = 1 },
		b = { .x = 2, .y = 2 },
		c = { .x = 3, .y = 3 };

	HASHMAP_INSERT(&hashmap, ka, a);
	HASHMAP_INSERT(&hashmap, kb, b);
	HASHMAP_INSERT(&hashmap, kc, c);

    ASSERT_EQ(hashmap_size(&hashmap), 3);

	hashmap_clear(&hashmap);

	ASSERT_FALSE(hashmap_has(&hashmap, &ka));
	ASSERT_FALSE(hashmap_has(&hashmap, &kb));
	ASSERT_FALSE(hashmap_has(&hashmap, &kc));

	ASSERT_EQ(hashmap_size(&hashmap), 0);
	ASSERT_EQ(hashmap_capacity(&hashmap), 4);

    hashmap_destroy(&hashmap);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(HASHMAP_POD, "HashMap POD Tests",
    setup_arena,
    teardown_arena,

    TEST_GROUP("Creation",
        TEST_NODE(test_pod_create),
        TEST_NODE(test_pod_create_macro),
        TEST_NODE(test_pod_destroy)
    ),

 	TEST_GROUP("Size",
		TEST_NODE(test_pod_grow)
	),

   TEST_GROUP("Copy",
        TEST_NODE(test_pod_copy),
        TEST_NODE(test_pod_copy_empty)
    ),

    TEST_GROUP("Access",
		TEST_NODE(test_pod_has),
        TEST_NODE(test_pod_at)
    ),

    TEST_GROUP("Modifiers",
        TEST_NODE(test_pod_insert),
		TEST_NODE(test_pod_insert_collision),
		TEST_NODE(test_pod_insert_grow),
        TEST_NODE(test_pod_remove),
        TEST_NODE(test_pod_clear)
    )
)

TEST_PROGRAM();
