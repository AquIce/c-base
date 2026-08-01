#include <base/foundation/core/test.h>
#include <base/foundation/containers/container.h>
#include <base/foundation/macros.h>
#include <base/foundation/containers/hashmap.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/arena.h>

#include <string.h>

// ============================================================
// TEST TYPES
// ============================================================

typedef struct {
    usize ctor_calls;
    usize dtor_calls;
    usize copy_calls;
    usize move_calls;
} TestTracker;

typedef struct {
	TestTracker tracker;
	usize len;
} TestKeyCtx;

typedef struct {
	char* str;
	usize len;
} TestKey;

typedef struct {
    i32 value;
} TestElem;

// ============================================================
// POLICY CALLBACKS
// ============================================================

internal void test_key_ctor(void* ctx, void* elem) {
    TestTracker* tracker = &((TestKeyCtx*)ctx)->tracker;
    TestKey* object = (TestKey*)elem;

    tracker->ctor_calls++;
	object->len = ((TestKeyCtx*)ctx)->len;
	object->str = malloc(object->len);
	assert(object->str);
}

internal void test_key_dtor(void* ctx, void* elem) {
    TestTracker* tracker = &((TestKeyCtx*)ctx)->tracker;
    TestKey* object = (TestKey*)elem;

    tracker->dtor_calls++;
	free(object->str);
	object->len = 0;
}

internal void test_key_copy(void* ctx, void* dest, const void* src) {
    TestTracker* tracker = &((TestKeyCtx*)ctx)->tracker;
    TestKey* dest_object = (TestKey*)dest;
	const TestKey* src_object = (const TestKey*)src;

    tracker->copy_calls++;
	dest_object->len = src_object->len;
	dest_object->str = malloc(dest_object->len);
	assert(dest_object->str);
	memcpy(dest_object->str, src_object, dest_object->len);
}

internal void test_key_move(void* ctx, void* dest, void* src) {
	LOG("Moving rn");
    TestTracker* tracker = &((TestKeyCtx*)ctx)->tracker;
    tracker->move_calls++;
    *(TestKey*)dest = *(TestKey*)src;
	*(TestKey*)src = (TestKey){
		.str = nullptr,
		.len = 0,
	};
}

internal bool test_key_equals(void* ctx, const void* elem, const void* other) {
	TestKey* elem_k = (TestKey*)elem;
	TestKey* other_k = (TestKey*)other;

	if(elem_k->len != other_k->len) {
		return false;
	}

	for(usize i = 0; i < elem_k->len; i++) {
		if(*(elem_k->str + i) != *(other_k->str + i)) {
			return false;
		}
	}

	return true;
}

internal hash_t test_key_hash(void* ctx, const void* object) {
	char* str = ((const TestKey*)object)->str;
	hash_t hash = 5381;
	for(usize i = 0; i < ((const TestKey*)object)->len; i++) {
		hash = ((hash << 5) + hash) + *(str + i);
	}
	return hash;
}

internal const ElementPolicy TEST_KEY_POLICY = {
	.ctor = &test_key_ctor,
	.dtor = &test_key_dtor,
	.copy = &test_key_copy,
	.move = &test_key_move,
	.equals = &test_key_equals,
	.hash = &test_key_hash,
};

// ---

internal void test_elem_ctor(void* ctx, void* elem) {
    TestTracker* tracker = ctx;
    TestElem* object = elem;

    tracker->ctor_calls++;
    object->value = 0;
}

internal void test_elem_dtor(void* ctx, void* elem) {
    TestTracker* tracker = ctx;
    tracker->dtor_calls++;
    (void)elem;
}

internal void test_elem_copy(void* ctx, void* dest, const void* src) {
    TestTracker* tracker = ctx;
    tracker->copy_calls++;
    *(TestElem*)dest = *(const TestElem*)src;
}

internal void test_elem_move(void* ctx, void* dest, void* src) {
    TestTracker* tracker = ctx;
    tracker->move_calls++;
    *(TestElem*)dest = *(TestElem*)src;
    test_elem_dtor(ctx, src);
}

internal const ElementPolicy TEST_ELEM_POLICY = {
    .ctor = &test_elem_ctor,
    .dtor = &test_elem_dtor,
    .copy = &test_elem_copy,
    .move = &test_elem_move,
};

// ============================================================
// FIXTURE
// ============================================================

internal MemorySource mem_source;
internal Allocator arena;

internal void setup_arena(void) {
    mem_source = malloc_memory_source_create();
    arena = arena_create(&mem_source, MB(1));
}

internal void teardown_arena(void) {
    arena_destroy(&arena);
}

// ============================================================
// HELPERS
// ============================================================

internal TestElem make_object(i32 value) {
    return (TestElem){ .value = value };
}

internal HashMap make_test_hashmap(
	usize capacity,
	TestKeyCtx* key_ctx,
	TestTracker* elem_tracker,
	ElementLifetime* key_lifetime,
	ElementLifetime* elem_lifetime
) {
    *key_lifetime = (ElementLifetime){
        .policy = &TEST_KEY_POLICY,
        .ctx = key_ctx,
    };
	*elem_lifetime = (ElementLifetime){
		.policy = &TEST_ELEM_POLICY,
		.ctx = elem_tracker,
	};

    return HASHMAP_CREATE_COMPLEX(
		TestKey,
		TestElem,
		&arena,
		capacity,
		HASHMAP_HASH_STRATEGRY_RECOMPUTE,
		key_lifetime,
		elem_lifetime
	);
}

// ============================================================
// TESTS
// ============================================================

TEST(test_policy_create) {
	TEST_SKIP();
	TestKeyCtx key_ctx = (TestKeyCtx){
		.len = 16,
		.tracker = (TestTracker){0}
	};
    TestTracker elem_tracker = {0};
	ElementLifetime key_lifetime;
	ElementLifetime elem_lifetime;

    HashMap hashmap = make_test_hashmap(16, &key_ctx, &elem_tracker, &key_lifetime, &elem_lifetime);

    ASSERT_EQ(hashmap_size(&hashmap), 0);
    ASSERT_EQ(hashmap_capacity(&hashmap), 16);
    ASSERT_EQ_PTR(hashmap.descriptor.key_lifetime, &key_lifetime);
    ASSERT_EQ_PTR(hashmap.descriptor.elem_lifetime, &elem_lifetime);

    hashmap_destroy(&hashmap);
}

TEST(test_policy_destroy) {
	TEST_SKIP();
	TestKeyCtx key_ctx = (TestKeyCtx){
		.len = 16,
		.tracker = (TestTracker){0}
	};
    TestTracker elem_tracker = {0};
	ElementLifetime key_lifetime;
	ElementLifetime elem_lifetime;

    HashMap hashmap = make_test_hashmap(16, &key_ctx, &elem_tracker, &key_lifetime, &elem_lifetime);

	TestKey k1, k2, k3;
	TEST_KEY_POLICY.ctor(&key_ctx, &k1);
	k1.len = 5; memcpy(k1.str, "Hello", k1.len);
	TEST_KEY_POLICY.ctor(&key_ctx, &k2);
	k2.len = 2; memcpy(k2.str, "Hi", k2.len);
	TEST_KEY_POLICY.ctor(&key_ctx, &k3);
	k3.len = 3; memcpy(k3.str, "Bar", k3.len);

	TestElem
		e1 = (TestElem){ .value = 1 },
		e2 = (TestElem){ .value = 3 },
		e3 = (TestElem){ .value = 2 };

	HASHMAP_INSERT(&hashmap, k1, e1);
	HASHMAP_INSERT(&hashmap, k2, e2);
	HASHMAP_INSERT(&hashmap, k3, e3);

	key_ctx.tracker.dtor_calls = 0;
	elem_tracker.dtor_calls = 0;

    hashmap_destroy(&hashmap);

    ASSERT_EQ(key_ctx.tracker.dtor_calls, 3);
    ASSERT_EQ(elem_tracker.dtor_calls, 3);
}

TEST_ROOT(HASHMAP_POLICY, "HashMap Policy Tests",
    setup_arena,
    teardown_arena,

	TEST_GROUP("Global",
		TEST_NODE(test_policy_create),
		TEST_NODE(test_policy_destroy),
	)
);

TEST_PROGRAM();
