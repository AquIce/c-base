#include <base/foundation/macros.h>
#include <base/foundation/core/test.h>
#include <base/foundation/memory/stack.h>
#include <base/foundation/memory/memory.h>

typedef struct {
    i32 x;
    i32 y;
    f32 z;
} TestStruct;

// ============================================================
// FIXTURE
// ============================================================

internal MemorySource source;
internal Allocator stack;

internal void setup(void) {
    source = malloc_memory_source_create();
    stack = stack_create(&source, KB(1));
}

internal void teardown(void) {
    stack_destroy(&stack);
    malloc_memory_source_destroy(&source);
}

internal void node_setup(void) {
	allocator_reset(&stack);
}

// ============================================================
// CREATION
// ============================================================

TEST(test_create) {
    ASSERT_NE_PTR(stack.handler, nullptr);
    ASSERT_NE_PTR(stack.vt, nullptr);
    ASSERT_EQ_PTR(stack.source, &source);

    stack_destroy(&stack);

    ASSERT_EQ_PTR(stack.handler, nullptr);
    ASSERT_EQ_PTR(stack.vt, nullptr);
    ASSERT_EQ_PTR(stack.source, nullptr);

    stack = stack_create(&source, KB(1));
}

// ============================================================
// ALLOCATION
// ============================================================

TEST(test_alloc_single) {
    i32* value = ALLOC(&stack, i32);

    ASSERT_NE_PTR(value, nullptr);

    *value = 42;

    ASSERT_EQ(*value, 42);
}

TEST(test_alloc_array) {
    i32* values = NALLOC(&stack, i32, 128);

    ASSERT_NE_PTR(values, nullptr);

    for(usize i = 0; i < 128; i++) {
        values[i] = (i32)i;
	}

    for(usize i = 0; i < 128; i++) {
        ASSERT_EQ(values[i], (i32)i);
	}
}

TEST(test_multiple_allocs) {
    TestStruct* a = ALLOC(&stack, TestStruct);
    TestStruct* b = ALLOC(&stack, TestStruct);
    TestStruct* c = ALLOC(&stack, TestStruct);

    ASSERT_NE_PTR(a, nullptr);
    ASSERT_NE_PTR(b, nullptr);
    ASSERT_NE_PTR(c, nullptr);

    ASSERT_TRUE(a != b);
    ASSERT_TRUE(a != c);
    ASSERT_TRUE(b != c);
}

// ============================================================
// ALIGNMENT
// ============================================================

TEST(test_alignment) {
    void* p8  = allocator_alloc(&stack, 16, 8);
    void* p16 = allocator_alloc(&stack, 16, 16);
    void* p32 = allocator_alloc(&stack, 16, 32);
    void* p64 = allocator_alloc(&stack, 16, 64);

    ASSERT_NE_PTR(p8, nullptr);
    ASSERT_NE_PTR(p16, nullptr);
    ASSERT_NE_PTR(p32, nullptr);
    ASSERT_NE_PTR(p64, nullptr);

    ASSERT_EQ((uptr)p8  % 8, 0);
    ASSERT_EQ((uptr)p16 % 16, 0);
    ASSERT_EQ((uptr)p32 % 32, 0);
    ASSERT_EQ((uptr)p64 % 64, 0);
}

// ============================================================
// CAPACITY
// ============================================================

TEST(test_out_of_memory) {
    Allocator local_stack = stack_create(&source, 128);

    void* first  = allocator_alloc(&local_stack, 100, 8);
    void* second = allocator_alloc(&local_stack, 100, 8);

    ASSERT_NE_PTR(first, nullptr);
    ASSERT_EQ_PTR(second, nullptr);

    stack_destroy(&local_stack);
}

TEST(test_out_of_memory_realloc) {
	Allocator local_stack = stack_create(&source, 128);

	void* first = allocator_alloc(&local_stack, 100, 1);
	ASSERT_NE_PTR(first, nullptr);

	ASSERT_DEATH(
		(void)allocator_realloc(&local_stack, first, 100, 200)
	);

	stack_destroy(&local_stack);
}

TEST(test_capacity_recovered_after_free) {
    Allocator local_stack = stack_create(&source, 128);

    void* first = allocator_alloc(&local_stack, 100, 8);

    ASSERT_NE_PTR(first, nullptr);
    ASSERT_EQ_PTR(
        allocator_alloc(&local_stack, 100, 8),
        nullptr
    );

    allocator_free(&local_stack, first);

    void* second = allocator_alloc(&local_stack, 100, 8);

    ASSERT_NE_PTR(second, nullptr);
    ASSERT_EQ_PTR(first, second);

    stack_destroy(&local_stack);
}

// ============================================================
// FREE
// ============================================================

TEST(test_free_last) {
    i32* first  = ALLOC(&stack, i32);
    i32* second = ALLOC(&stack, i32);

    allocator_free(&stack, second);

    i32* third = ALLOC(&stack, i32);

    ASSERT_EQ_PTR(second, third);
}

TEST(test_multiple_free) {
    i32* a = ALLOC(&stack, i32);
    i32* b = ALLOC(&stack, i32);
    i32* c = ALLOC(&stack, i32);

    allocator_free(&stack, c);
    allocator_free(&stack, b);
    allocator_free(&stack, a);

    i32* d = ALLOC(&stack, i32);

    ASSERT_EQ_PTR(a, d);
}

TEST(test_free_middle_fails) {
    i32* a = ALLOC(&stack, i32);
    i32* b = ALLOC(&stack, i32);

	ASSERT_DEATH(
		(void)allocator_free(&stack, a)
	);

    (void)b;
}

// ============================================================
// REALLOC
// ============================================================

TEST(test_realloc_last) {
	(void)ALLOC(&stack, i32);

    i32* second = ALLOC(&stack, i32);
    ASSERT_NE_PTR(second, nullptr);

    i32* third = allocator_realloc(
		&stack,
		second,
		sizeof(i32),
		sizeof(i32) * 3
	);

    ASSERT_NE_PTR(third, nullptr);
    ASSERT_EQ_PTR(second, third);

	for(i32 i = 0; i < 3; i++) {
		*(third + i) = i + 1;
	}

    ASSERT_EQ(*third, 1);
    ASSERT_EQ(*(third + 1), 2);
    ASSERT_EQ(*(third + 2), 3);

    i32* next = ALLOC(&stack, i32);
    ASSERT_NE_PTR(next, nullptr);

    ASSERT_TRUE((u8*)next >= (u8*)third + sizeof(i32) * 3);
}

TEST(test_realloc_last_smaller) {
	(void)ALLOC(&stack, i32);

    i32* second = ALLOC(&stack, i32);
    ASSERT_NE_PTR(second, nullptr);

    i8* third = allocator_realloc(
		&stack,
		second,
		sizeof(i32),
		sizeof(i8)
	);

    ASSERT_NE_PTR(third, nullptr);
    ASSERT_EQ_PTR(second, (i32*)third);

    *third = 127;

    ASSERT_EQ(*third, 127);

    i32* next = ALLOC(&stack, i32);
    ASSERT_NE_PTR(next, nullptr);

    ASSERT_TRUE((u8*)next >= (u8*)third + sizeof(i8));
}

TEST(test_realloc_last_as_free) {
    i32* first  = ALLOC(&stack, i32);

    void* ptr = allocator_realloc(&stack, first, sizeof(i32), 0);

    ASSERT_EQ_PTR(ptr, nullptr);

	i32* second = ALLOC(&stack, i32);

    ASSERT_EQ_PTR(first, second);
}

TEST(test_realloc_middle_fails) {
    i32* a = ALLOC(&stack, i32);
    i32* b = ALLOC(&stack, i32);

	ASSERT_DEATH(
		(void)allocator_realloc(&stack, a, sizeof(i32), sizeof(TestStruct))
	);

    (void)b;
}

// ============================================================
// RESET
// ============================================================

TEST(test_reset) {
    void* first = allocator_alloc(&stack, 128, 8);

    allocator_reset(&stack);

    void* second = allocator_alloc(&stack, 128, 8);

    ASSERT_EQ_PTR(first, second);
}

// ============================================================
// MISC
// ============================================================

TEST(test_source_association) {
    ASSERT_EQ_PTR(stack.source, &source);
}

// ============================================================
// ROOT
// ============================================================

TEST_ROOT(STACK, "Stack Allocator Tests",
    setup,
    teardown,

    TEST_GROUP("Creation",
        TEST_NODE(test_create)
    ),

    TEST_GROUP("Allocation",
        TEST_NODE(test_alloc_single),
        TEST_NODE(test_alloc_array),
        TEST_NODE(test_multiple_allocs)
    ),

    TEST_GROUP("Alignment",
        TEST_NODE(test_alignment)
    ),

    TEST_GROUP("Capacity",
        TEST_NODE(test_out_of_memory),
        TEST_NODE(test_out_of_memory_realloc),
        TEST_NODE(test_capacity_recovered_after_free)
    ),

    TEST_GROUP("Free",
        TEST_NODE(test_free_last),
        TEST_NODE(test_multiple_free),
        TEST_NODE(test_free_middle_fails)
    ),

	TEST_GROUP("Realloc",
        TEST_NODE(test_realloc_last),
        TEST_NODE(test_realloc_last_smaller),
        TEST_NODE(test_realloc_last_as_free),
        TEST_NODE_SETUP(test_realloc_middle_fails, node_setup)
    ),

    TEST_GROUP("Reset",
        TEST_NODE_SETUP(test_reset, node_setup)
    ),

    TEST_GROUP("Misc",
        TEST_NODE(test_source_association)
    )
);

TEST_PROGRAM();
