#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>
#include <base/foundation/containers/dynarray.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/arena.h>

// ============================================================
// TEST TYPES
// ============================================================

typedef struct {
    i32 value;
} TestObject;

typedef struct {
    usize ctor_calls;
    usize dtor_calls;
    usize copy_calls;
    usize move_calls;
} TestTracker;

// ============================================================
// POLICY CALLBACKS
// ============================================================

internal void test_ctor(void* ctx, void* elem) {
    TestTracker* tracker = ctx;
    TestObject* object = elem;

    tracker->ctor_calls++;
    object->value = 0;
}

internal void test_dtor(void* ctx, void* elem) {
    TestTracker* tracker = ctx;
    tracker->dtor_calls++;
    (void)elem;
}

internal void test_copy(void* ctx, void* dest, const void* src) {
    TestTracker* tracker = ctx;
    tracker->copy_calls++;
    *(TestObject*)dest = *(const TestObject*)src;
}

internal void test_move(void* ctx, void* dest, void* src) {
    TestTracker* tracker = ctx;
    tracker->move_calls++;
    *(TestObject*)dest = *(TestObject*)src;
    test_dtor(ctx, src);
}

internal const ElementPolicy TEST_POLICY = {
    .ctor = test_ctor,
    .dtor = test_dtor,
    .copy = test_copy,
    .move = test_move,
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

internal TestObject make_object(i32 value) {
    return (TestObject){ .value = value };
}

internal DynArray make_test_array(usize capacity, TestTracker* tracker, ElementLifetime* lifetime) {
    *lifetime = (ElementLifetime){
        .policy = &TEST_POLICY,
        .ctx = tracker,
    };

    return DYNARRAY_CREATE_COMPLEX(TestObject, &arena, capacity, lifetime);
}

// ============================================================
// TESTS
// ============================================================

TEST(test_policy_create) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(16, &tracker, &lifetime);

    ASSERT_EQ(dynarray_size(&array), 0);
    ASSERT_EQ(dynarray_capacity(&array), 16);
    ASSERT_EQ_PTR(array.descriptor.elem_lifetime, &lifetime);

    dynarray_destroy(&array);
}

TEST(test_policy_destroy) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(4, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(2));
    DYNARRAY_PUSH(&array, make_object(3));

	tracker.dtor_calls = 0;

    dynarray_destroy(&array);

    ASSERT_EQ(tracker.dtor_calls, 3);
}

TEST(test_policy_ctor_called) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(4, &tracker, &lifetime);

    ASSERT_TRUE(dynarray_resize(&array, 4));

    ASSERT_EQ(tracker.ctor_calls, 4);

    for(usize i = 0; i < dynarray_size(&array); i++) {
        ASSERT_EQ(DYNARRAY_AT(&array, TestObject, i)->value, 0);
    }

    dynarray_destroy(&array);
}

TEST(test_policy_dtor_called) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(4, &tracker, &lifetime);

    ASSERT_TRUE(dynarray_resize(&array, 4));

    tracker.dtor_calls = 0;

    ASSERT_TRUE(dynarray_resize(&array, 1));

    ASSERT_EQ(tracker.dtor_calls, 3);

    dynarray_destroy(&array);
}

TEST(test_policy_copy) {
    TestTracker tracker = {0};

    const ElementPolicy policy = {
        .ctor = test_ctor,
        .dtor = test_dtor,
        .copy = test_copy,
        .move = nullptr,
    };

    ElementLifetime lifetime = {
        .policy = &policy,
        .ctx = &tracker,
    };

    DynArray array = DYNARRAY_CREATE_COMPLEX(TestObject, &arena, 2, &lifetime);

    TestObject obj = make_object(123);

    tracker.copy_calls = 0;

    ASSERT_TRUE(dynarray_push(&array, &obj));

    ASSERT_EQ(tracker.copy_calls, 1);
    ASSERT_EQ(DYNARRAY_AT(&array, TestObject, 0)->value, 123);

    dynarray_destroy(&array);
}

TEST(test_policy_copy_rejected) {
    TestTracker tracker = {0};

    const ElementPolicy policy = {
        .ctor = test_ctor,
        .dtor = test_dtor,
        .copy = nullptr,
        .move = nullptr,
    };

    ElementLifetime lifetime = {
        .policy = &policy,
        .ctx = &tracker,
    };

    DynArray array = DYNARRAY_CREATE_COMPLEX(TestObject, &arena, 2, &lifetime);

    TestObject obj = make_object(42);

    ASSERT_TRUE(!dynarray_push(&array, &obj));

    ASSERT_EQ(dynarray_size(&array), 0);
    ASSERT_EQ(tracker.copy_calls, 0);

    dynarray_destroy(&array);
}

TEST(test_policy_move) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(2, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(2));

    ASSERT_TRUE(dynarray_reserve(&array, 8));
    ASSERT_TRUE(tracker.move_calls > 0);

    dynarray_destroy(&array);
}

TEST(test_policy_move_invalid) {
    TestTracker tracker = {0};

    const ElementPolicy policy = {
        .ctor = test_ctor,
        .dtor = test_dtor,
        .copy = nullptr,
        .move = nullptr,
    };

    ElementLifetime lifetime = {
        .policy = &policy,
        .ctx = &tracker,
    };

    DynArray array = DYNARRAY_CREATE_COMPLEX(TestObject, &arena, 4, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));

    ASSERT_DEATH(dynarray_remove(&array, 0));

    dynarray_destroy(&array);
}

TEST(test_policy_reserve_move) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(1, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(10));
    DYNARRAY_PUSH(&array, make_object(20));

    tracker.move_calls = 0;

    ASSERT_TRUE(dynarray_reserve(&array, 32));
    ASSERT_TRUE(tracker.move_calls > 0);

    dynarray_destroy(&array);
}

TEST(test_policy_reserve_copy_fallback) {
    TestTracker tracker = {0};

    const ElementPolicy policy = {
        .ctor = test_ctor,
        .dtor = test_dtor,
        .copy = test_copy,
        .move = nullptr,
    };

    ElementLifetime lifetime = {
        .policy = &policy,
        .ctx = &tracker,
    };

    DynArray array = DYNARRAY_CREATE_COMPLEX(TestObject, &arena, 1, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(2));

    tracker.copy_calls = 0;

    ASSERT_TRUE(dynarray_reserve(&array, 16));
    ASSERT_TRUE(tracker.copy_calls > 0);
    ASSERT_EQ(tracker.move_calls, 0);

    dynarray_destroy(&array);
}

TEST(test_policy_push) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(0, &tracker, &lifetime);

    ASSERT_TRUE(DYNARRAY_PUSH(&array, make_object(42)));

    ASSERT_EQ(dynarray_size(&array), 1);

    const TestObject* obj = DYNARRAY_AT(&array, TestObject, 0);
    ASSERT_EQ(obj->value, 42);

    dynarray_destroy(&array);
}

TEST(test_policy_pop) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(2, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(2));

	tracker.dtor_calls = 0;

    dynarray_pop(&array);

    ASSERT_EQ(dynarray_size(&array), 1);
    ASSERT_EQ(tracker.dtor_calls, 1);

    dynarray_destroy(&array);
}

TEST(test_policy_insert) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(0, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(3));

    TestObject middle = make_object(2);

    ASSERT_TRUE(dynarray_insert(&array, 1, &middle));

    ASSERT_EQ(dynarray_size(&array), 3);

    ASSERT_EQ(DYNARRAY_AT(&array, TestObject, 0)->value, 1);
    ASSERT_EQ(DYNARRAY_AT(&array, TestObject, 1)->value, 2);
    ASSERT_EQ(DYNARRAY_AT(&array, TestObject, 2)->value, 3);

    dynarray_destroy(&array);
}

TEST(test_policy_remove) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(0, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(2));
    DYNARRAY_PUSH(&array, make_object(3));
    DYNARRAY_PUSH(&array, make_object(4));

	tracker.dtor_calls = 0;

    ASSERT_TRUE(dynarray_remove(&array, 1));

    ASSERT_EQ(dynarray_size(&array), 3);
    ASSERT_TRUE(tracker.dtor_calls >= 1);

    dynarray_destroy(&array);
}

TEST(test_policy_resize_grow_ctor) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(5, &tracker, &lifetime);

    ASSERT_TRUE(dynarray_resize(&array, 5));
    ASSERT_EQ(tracker.ctor_calls, 5);

    dynarray_destroy(&array);
}

TEST(test_policy_resize_shrink_dtor) {
    TestTracker tracker = {0};
    ElementLifetime lifetime;

    DynArray array = make_test_array(0, &tracker, &lifetime);

    DYNARRAY_PUSH(&array, make_object(1));
    DYNARRAY_PUSH(&array, make_object(2));
    DYNARRAY_PUSH(&array, make_object(3));

	tracker.dtor_calls = 0;

    ASSERT_TRUE(dynarray_resize(&array, 1));
    ASSERT_EQ(tracker.dtor_calls, 2);

    dynarray_destroy(&array);
}

TEST_ROOT(DYNARRAY_POLICY, "DynArray Policy Tests",
    setup_arena,
    teardown_arena,

	TEST_GROUP("Global",
		TEST_NODE(test_policy_create),
		TEST_NODE(test_policy_destroy),
	),

	TEST_GROUP("Base Members",
		TEST_NODE(test_policy_ctor_called),
		TEST_NODE(test_policy_dtor_called),
		TEST_NODE(test_policy_copy),
		TEST_NODE(test_policy_copy_rejected),
		TEST_NODE(test_policy_move),
		TEST_NODE(test_policy_move_invalid),
	),

	TEST_GROUP("Reserve",
		TEST_NODE(test_policy_reserve_move),
		TEST_NODE(test_policy_reserve_copy_fallback),
	),

	TEST_GROUP("Resize",
		TEST_NODE(test_policy_resize_grow_ctor),
		TEST_NODE(test_policy_resize_shrink_dtor),
	),

	TEST_GROUP("Modifiers",
		TEST_NODE(test_policy_push),
		TEST_NODE(test_policy_pop),
		TEST_NODE(test_policy_insert),
		TEST_NODE(test_policy_remove)
	)
);

TEST_PROGRAM();
