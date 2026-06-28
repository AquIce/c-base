#include <assert.h>
#include <stdio.h>

#include <base/foundation/containers/dynarray.h>
#include <base/foundation/memory/allocator.h>
#include <base/foundation/macros.h>

// ============================================================
// POLICY TEST TYPES
// ============================================================

typedef struct {
    int value;
    int ctor_calls;
    int dtor_calls;
    int copy_calls;
    int move_calls;
} TestObject;

// ============================================================
// UTILITIES
// ============================================================

internal void* test_alloc(void* ctx, usize size, usize alignment);
internal void test_free(void* ctx, void* ptr);
Allocator make_test_allocator(void);

internal ElementPolicy make_test_policy(TestObject* tracker);

// ============================================================
// CREATION / DESTRUCTION
// ============================================================

internal void test_policy_create(void);
internal void test_policy_destroy(void);

// ============================================================
// CTOR / DTOR BEHAVIOR
// ============================================================

internal void test_policy_ctor_called(void);
internal void test_policy_dtor_called(void);

// ============================================================
// COPY SEMANTICS
// ============================================================

internal void test_policy_copy(void);
internal void test_policy_copy_rejected(void);

// ============================================================
// MOVE SEMANTICS
// ============================================================

internal void test_policy_move(void);
internal void test_policy_move_invalid(void);

// ============================================================
// RESERVE / REALLOCATION
// ============================================================

internal void test_policy_reserve_move_path(void);

// ============================================================
// RESIZE (FUTURE)
// ============================================================

internal void test_policy_resize_grow_ctor(void);
internal void test_policy_resize_shrink_dtor(void);

// ============================================================
// MODIFIERS
// ============================================================

internal void test_policy_push(void);
internal void test_policy_pop(void);
internal void test_policy_insert(void);
internal void test_policy_remove(void);

// ============================================================
// MAIN
// ============================================================

int main(void) {
    test_policy_create();
    test_policy_destroy();

    test_policy_ctor_called();
    test_policy_dtor_called();

    test_policy_copy();
    test_policy_copy_rejected();

    test_policy_move();
    test_policy_move_invalid();

    test_policy_reserve_move_path();

    test_policy_resize_grow_ctor();
    test_policy_resize_shrink_dtor();

    test_policy_push();
    test_policy_pop();
    test_policy_insert();
    test_policy_remove();

    printf("\n[POLICY TESTS PASSED]\n");
    return 0;
}
