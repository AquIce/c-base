#include <stdio.h>
#include <assert.h>

#include <base/foundation/memory/allocator.h>

static void test_allocator_exists(void) {
    Allocator *alloc = NULL;

    (void)alloc;

    printf("Allocator type is visible.\n");
}

int main(void) {
    test_allocator_exists();

    printf("Allocator test passed.\n");
    return 0;
}
