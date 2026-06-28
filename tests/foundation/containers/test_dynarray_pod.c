#include <assert.h>
#include <stdio.h>

#include <base/foundation/containers/dynarray.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/macros.h>

// ============================================================
// POD TEST TYPES
// ============================================================

typedef struct {
    int x;
    int y;
} TestPOD;

// ============================================================
// UTILITIES
// ============================================================

internal void* test_alloc(void* ctx, usize size, usize alignment);
internal void test_free(void* ctx, void* ptr);
Allocator make_test_allocator(void);

// ============================================================
// CREATION / DESTRUCTION
// ============================================================

internal void test_pod_create(void);
internal void test_pod_destroy(void);

// ============================================================
// RESERVE / CAPACITY
// ============================================================

internal void test_pod_reserve_growth(void);
internal void test_pod_reserve_noop(void);

// ============================================================
// COPY (MEMCPY PATH)
// ============================================================

internal void test_pod_copy(void);
internal void test_pod_copy_empty(void);

// ============================================================
// ELEMENT ACCESS
// ============================================================

internal void test_pod_at(void);
internal void test_pod_front_back(void);
internal void test_pod_data(void);

// ============================================================
// MODIFIERS
// ============================================================

internal void test_pod_push(void);
internal void test_pod_pop(void);
internal void test_pod_insert(void);
internal void test_pod_remove(void);

internal void test_pod_append(void);
internal void test_pod_clear(void);
internal void test_pod_reset(void);

// ============================================================
// EDGE CASES
// ============================================================

internal void test_pod_zero_capacity(void);
internal void test_pod_large_reserve(void);

// ============================================================
// MAIN
// ============================================================

int main(void) {
    test_pod_create();
    test_pod_destroy();

    test_pod_reserve_growth();
    test_pod_reserve_noop();

    test_pod_copy();
    test_pod_copy_empty();

    test_pod_at();
    test_pod_front_back();
    test_pod_data();

    test_pod_push();
    test_pod_pop();
    test_pod_insert();
    test_pod_remove();

    test_pod_append();
    test_pod_clear();
    test_pod_reset();

    test_pod_zero_capacity();
    test_pod_large_reserve();

    printf("\n[POD TESTS PASSED]\n");
    return 0;
}
