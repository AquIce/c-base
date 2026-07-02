#include <base/foundation/core/test.h>
#include <base/foundation/memory/allocator.h>

TEST(test_allocator_exists) {
    Allocator *alloc = nullptr;
    (void)alloc;
}

TEST_ROOT(ALLOCATOR, "Allocator",
	nullptr, nullptr,
	TEST_NODE(test_allocator_exists)
);
TEST_PROGRAM();
