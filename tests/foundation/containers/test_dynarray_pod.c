#include <base/foundation/core/test.h>
#include <base/foundation/macros.h>

#include <base/foundation/containers/dynarray.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>

#include <stdio.h>

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
    DynArray arr = dynarray_create(&arena, 4, 1, 1);

    ASSERT_TRUE(arr.buffer != nullptr);
    ASSERT_EQ(arr.size, 0);

    ASSERT_EQ(arr.descriptor.capacity, 4);
    ASSERT_EQ(arr.descriptor.elem_size, 1);
    ASSERT_EQ(arr.descriptor.alignment, 1);
    ASSERT_EQ_PTR(arr.descriptor.allocator, &arena);
    ASSERT_EQ_PTR(arr.descriptor.elem_lifetime, nullptr);
}

TEST(test_pod_create_macro) {
    DynArray arr = DYNARRAY_CREATE(char, &arena, 4);

    ASSERT_TRUE(arr.buffer != nullptr);
    ASSERT_EQ(arr.size, 0);

    ASSERT_EQ(arr.descriptor.capacity, 4);
    ASSERT_EQ(arr.descriptor.elem_size, sizeof(char));
    ASSERT_EQ(arr.descriptor.alignment, _Alignof(char));
    ASSERT_EQ_PTR(arr.descriptor.allocator, &arena);
    ASSERT_EQ_PTR(arr.descriptor.elem_lifetime, nullptr);
}

TEST(test_pod_destroy) {
    DynArray arr = dynarray_create(&arena, 4, sizeof(i32), _Alignof(i32));

    dynarray_destroy(&arr);

    ASSERT_EQ_PTR(arr.buffer, nullptr);
    ASSERT_EQ(arr.size, 0);

    ASSERT_EQ(arr.descriptor.capacity, 0);
    ASSERT_EQ(arr.descriptor.elem_size, 0);
    ASSERT_EQ(arr.descriptor.alignment, 0);
    ASSERT_EQ_PTR(arr.descriptor.allocator, nullptr);
    ASSERT_EQ_PTR(arr.descriptor.elem_lifetime, nullptr);
}

// ============================================================
// RESERVE
// ============================================================

TEST(test_pod_reserve_growth) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 2);

    void* old = arr.buffer;

    ASSERT_TRUE(dynarray_reserve(&arr, 8));

    ASSERT_TRUE(arr.buffer != nullptr);
    ASSERT_TRUE(arr.buffer != old);

    ASSERT_EQ(arr.size, 0);
    ASSERT_TRUE(arr.descriptor.capacity >= 8);

    dynarray_destroy(&arr);
}

TEST(test_pod_reserve_noop) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 8);

    void* old = arr.buffer;

    ASSERT_TRUE(dynarray_reserve(&arr, 4));

    ASSERT_EQ_PTR(arr.buffer, old);
    ASSERT_EQ(arr.size, 0);
    ASSERT_EQ(arr.descriptor.capacity, 8);

    dynarray_destroy(&arr);
}

// ============================================================
// RESIZE
// ============================================================

TEST(test_pod_resize_grow) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 8);

    ASSERT_TRUE(dynarray_resize(&arr, 5));

    ASSERT_EQ(arr.size, 5);
    ASSERT_EQ(arr.descriptor.capacity, 8);

    dynarray_destroy(&arr);
}

TEST(test_pod_resize_past_capacity) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 2);

    ASSERT_TRUE(!dynarray_resize(&arr, 5));

    ASSERT_EQ(arr.size, 0);
    ASSERT_EQ(arr.descriptor.capacity, 2);

    dynarray_destroy(&arr);
}

TEST(test_pod_resize_shrink) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 8);

    DYNARRAY_PUSH(&arr, 1);
    DYNARRAY_PUSH(&arr, 2);
    DYNARRAY_PUSH(&arr, 3);
    DYNARRAY_PUSH(&arr, 4);
    DYNARRAY_PUSH(&arr, 5);

    usize cap = arr.descriptor.capacity;

    ASSERT_TRUE(dynarray_resize(&arr, 2));

    ASSERT_EQ(arr.size, 2);
    ASSERT_EQ(arr.descriptor.capacity, cap);

    ASSERT_EQ(*(i32*)dynarray_at(&arr, 0), 1);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 1), 2);

    dynarray_destroy(&arr);
}

TEST(test_pod_resize_same_size) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&arr, 10);
    DYNARRAY_PUSH(&arr, 20);

    void* buf = arr.buffer;
    usize cap = arr.descriptor.capacity;

    ASSERT_TRUE(dynarray_resize(&arr, 2));

    ASSERT_EQ_PTR(arr.buffer, buf);
    ASSERT_EQ(arr.size, 2);
    ASSERT_EQ(arr.descriptor.capacity, cap);

    dynarray_destroy(&arr);
}

TEST(test_pod_resize_zero) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&arr, 1);
    DYNARRAY_PUSH(&arr, 2);
    DYNARRAY_PUSH(&arr, 3);

    usize cap = arr.descriptor.capacity;

    ASSERT_TRUE(dynarray_resize(&arr, 0));

    ASSERT_EQ(arr.size, 0);
    ASSERT_EQ(arr.descriptor.capacity, cap);

    dynarray_destroy(&arr);
}

// ============================================================
// COPY
// ============================================================

TEST(test_pod_copy) {
    DynArray src = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&src, 10);
    DYNARRAY_PUSH(&src, 20);
    DYNARRAY_PUSH(&src, 30);

    DynArray dst = {0};

    ASSERT_TRUE(dynarray_copy(&dst, &src));

    ASSERT_TRUE(dst.buffer != nullptr);
    ASSERT_TRUE(dst.buffer != src.buffer);

    ASSERT_EQ(dst.size, src.size);

    ASSERT_EQ(dst.descriptor.capacity, src.descriptor.capacity);
    ASSERT_EQ(dst.descriptor.elem_size, src.descriptor.elem_size);
    ASSERT_EQ(dst.descriptor.alignment, src.descriptor.alignment);
    ASSERT_EQ_PTR(dst.descriptor.allocator, src.descriptor.allocator);
    ASSERT_EQ_PTR(dst.descriptor.elem_lifetime, src.descriptor.elem_lifetime);

    ASSERT_EQ(*(i32*)dynarray_at(&dst, 0), 10);
    ASSERT_EQ(*(i32*)dynarray_at(&dst, 1), 20);
    ASSERT_EQ(*(i32*)dynarray_at(&dst, 2), 30);

    dynarray_destroy(&dst);
    dynarray_destroy(&src);
}

TEST(test_pod_copy_empty) {
    DynArray src = DYNARRAY_CREATE(i32, &arena, 4);
    DynArray dst = {0};

    ASSERT_TRUE(dynarray_copy(&dst, &src));

    ASSERT_TRUE(dst.buffer != src.buffer);
    ASSERT_EQ(dst.size, 0);

    ASSERT_EQ(dst.descriptor.capacity, src.descriptor.capacity);
    ASSERT_EQ(dst.descriptor.elem_size, src.descriptor.elem_size);
    ASSERT_EQ(dst.descriptor.alignment, src.descriptor.alignment);
    ASSERT_EQ_PTR(dst.descriptor.allocator, src.descriptor.allocator);
    ASSERT_EQ_PTR(dst.descriptor.elem_lifetime, src.descriptor.elem_lifetime);

    dynarray_destroy(&dst);
    dynarray_destroy(&src);
}

// ============================================================
// ACCESS
// ============================================================

TEST(test_pod_at) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&arr, 1);
    DYNARRAY_PUSH(&arr, 2);
    DYNARRAY_PUSH(&arr, 3);

    ASSERT_EQ(*(i32*)dynarray_at(&arr, 0), 1);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 1), 2);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 2), 3);

    dynarray_destroy(&arr);
}

TEST(test_pod_front_back) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&arr, 5);
    DYNARRAY_PUSH(&arr, 10);
    DYNARRAY_PUSH(&arr, 15);

    ASSERT_EQ(*(i32*)dynarray_front(&arr), 5);
    ASSERT_EQ(*(i32*)dynarray_back(&arr), 15);

    dynarray_destroy(&arr);
}

TEST(test_pod_data) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&arr, 42);
    DYNARRAY_PUSH(&arr, 99);

    i32* data = dynarray_data(&arr);

    ASSERT_TRUE(data != nullptr);
    ASSERT_EQ_PTR(data, arr.buffer);

    ASSERT_EQ(data[0], 42);
    ASSERT_EQ(data[1], 99);

    dynarray_destroy(&arr);
}

// ============================================================
// MODIFIERS
// ============================================================

TEST(test_pod_push) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 2);

    i32 a = 10, b = 20, c = 30;

    ASSERT_TRUE(dynarray_push(&arr, &a));
    ASSERT_TRUE(dynarray_push(&arr, &b));
    ASSERT_TRUE(dynarray_push(&arr, &c));

    ASSERT_EQ(dynarray_size(&arr), 3);

    ASSERT_EQ(*(i32*)dynarray_at(&arr, 0), 10);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 1), 20);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 2), 30);

    dynarray_destroy(&arr);
}

TEST(test_pod_pop) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    i32 a = 1, b = 2, c = 3;

    dynarray_push(&arr, &a);
    dynarray_push(&arr, &b);
    dynarray_push(&arr, &c);

    dynarray_pop(&arr);

    ASSERT_EQ(dynarray_size(&arr), 2);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 0), 1);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 1), 2);

    dynarray_destroy(&arr);
}

TEST(test_pod_insert) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 2);

    i32 a = 1, b = 3, mid = 2;

    dynarray_push(&arr, &a);
    dynarray_push(&arr, &b);

    dynarray_insert(&arr, 1, &mid);

    ASSERT_EQ(dynarray_size(&arr), 3);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 0), 1);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 1), 2);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 2), 3);

    dynarray_destroy(&arr);
}

TEST(test_pod_remove) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    i32 v[] = {1, 2, 3, 4};

    for (i32 i = 0; i < 4; i++) {
        dynarray_push(&arr, &v[i]);
    }

    dynarray_remove(&arr, 1);

    ASSERT_EQ(dynarray_size(&arr), 3);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 0), 1);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 1), 3);
    ASSERT_EQ(*(i32*)dynarray_at(&arr, 2), 4);

    dynarray_destroy(&arr);
}

TEST(test_pod_append) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    i32 a[] = {1, 2, 3};
    i32 b[] = {4, 5};

    dynarray_append(&arr, a, 3);
    dynarray_append(&arr, b, 2);

    ASSERT_EQ(dynarray_size(&arr), 5);

    for (i32 i = 0; i < 5; i++) {
        ASSERT_EQ(*(i32*)dynarray_at(&arr, i), i + 1);
    }

    dynarray_destroy(&arr);
}

TEST(test_pod_clear) {
    DynArray arr = DYNARRAY_CREATE(i32, &arena, 4);

    i32 v[] = {10, 20, 30};

    for (i32 i = 0; i < 3; i++) {
        dynarray_push(&arr, &v[i]);
    }

    dynarray_clear(&arr);

    ASSERT_EQ(dynarray_size(&arr), 0);
    ASSERT_EQ(dynarray_capacity(&arr), 4);

    dynarray_destroy(&arr);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(DYNARRAY_POD, "DynArray POD Tests",
    setup_arena,
    teardown_arena,

    TEST_GROUP("Creation",
        TEST_NODE(test_pod_create),
        TEST_NODE(test_pod_create_macro),
        TEST_NODE(test_pod_destroy)
    ),

    TEST_GROUP("Reserve",
        TEST_NODE(test_pod_reserve_growth),
        TEST_NODE(test_pod_reserve_noop)
    ),

    TEST_GROUP("Resize",
        TEST_NODE(test_pod_resize_grow),
        TEST_NODE(test_pod_resize_past_capacity),
        TEST_NODE(test_pod_resize_shrink),
        TEST_NODE(test_pod_resize_same_size),
        TEST_NODE(test_pod_resize_zero)
    ),

    TEST_GROUP("Copy",
        TEST_NODE(test_pod_copy),
        TEST_NODE(test_pod_copy_empty)
    ),

    TEST_GROUP("Access",
        TEST_NODE(test_pod_at),
        TEST_NODE(test_pod_front_back),
        TEST_NODE(test_pod_data)
    ),

    TEST_GROUP("Modifiers",
        TEST_NODE(test_pod_push),
        TEST_NODE(test_pod_pop),
        TEST_NODE(test_pod_insert),
        TEST_NODE(test_pod_remove),
        TEST_NODE(test_pod_append),
        TEST_NODE(test_pod_clear)
    )
)

TEST_PROGRAM();
