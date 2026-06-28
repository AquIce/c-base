#include <assert.h>
#include <stdio.h>

#include <base/foundation/containers/dynarray.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/macros.h>

/*
Capacity / size boundaries
	size = 0
	capacity = 0
	size == capacity
	resizing to exactly current size
	resizing to 1 element
Extreme operations
	pushing into empty array
	popping last element
	inserting at index 0 or size
	removing first/last element
Invalid or dangerous states
	null allocator (if allowed at all)
	missing copy or move in policy
	self-copy / self-move (dynarray_copy(&a, &a))
	double destroy
	reset after move
Memory boundary behavior
	reserve to same capacity (should be no-op)
	reserve just above capacity (forces realloc)
	shrink-to-fit to zero or minimal size
*/

// ============================================================
// UTILITIES
// ============================================================

internal void* test_alloc(void* ctx, usize size, usize alignment);
internal void test_free(void* ctx, void* ptr);
Allocator make_test_allocator(void);

// ============================================================
// INVALID CONFIGS
// ============================================================

internal void test_null_allocator(void);
internal void test_null_policy_copy_missing(void);
internal void test_null_policy_move_missing(void);

// ============================================================
// BOUNDARY CONDITIONS
// ============================================================

internal void test_zero_capacity(void);
internal void test_one_element(void);
internal void test_full_to_empty_cycle(void);

// ============================================================
// RESIZE EDGE CASES
// ============================================================

internal void test_resize_to_same_size(void);
internal void test_resize_to_zero(void);
internal void test_resize_beyond_capacity(void);

// ============================================================
// COPY / MOVE EDGE CASES
// ============================================================

internal void test_copy_into_self(void);
internal void test_move_into_self(void);
internal void test_copy_empty(void);

// ============================================================
// MEMORY SAFETY BEHAVIOR
// ============================================================

internal void test_double_destroy(void);
internal void test_reset_after_move(void);

// ============================================================
// MAIN
// ============================================================

int main(void) {
    test_null_allocator();
    test_null_policy_copy_missing();
    test_null_policy_move_missing();

    test_zero_capacity();
    test_one_element();
    test_full_to_empty_cycle();

    test_resize_to_same_size();
    test_resize_to_zero();
    test_resize_beyond_capacity();

    test_copy_into_self();
    test_move_into_self();
    test_copy_empty();

    test_double_destroy();
    test_reset_after_move();

    printf("\n[EDGE TESTS PASSED]\n");
    return 0;
}
