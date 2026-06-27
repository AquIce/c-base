#include <assert.h>
#include <stdio.h>

#include <base/foundation/macros.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>

typedef struct TestStruct {
    i32 x;
    i32 y;
    f32 z;
} TestStruct;

internal void test_create(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    assert(arena.handler != nullptr);
    assert(arena.vt != nullptr);
    assert(arena.source == &source);

    arena_destroy(&arena);

    assert(arena.handler == nullptr);
    assert(arena.vt == nullptr);
    assert(arena.source == nullptr);

    malloc_memory_source_destroy(&source);

    printf("[PASS] create\n");
}

internal void test_alloc_single(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    i32* value = ALLOC(&arena, i32);

    assert(value != nullptr);

    *value = 42;

    assert(*value == 42);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] alloc single\n");
}

internal void test_alloc_array(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    i32* values = NALLOC(&arena, i32, 128);

    assert(values != nullptr);

    for(usize i = 0; i < 128; ++i) {
        values[i] = (i32)i;
    }

    for(usize i = 0; i < 128; ++i) {
        assert(values[i] == (i32)i);
    }

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] alloc array\n");
}

internal void test_multiple_allocs(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    TestStruct* a = ALLOC(&arena, TestStruct);
    TestStruct* b = ALLOC(&arena, TestStruct);
    TestStruct* c = ALLOC(&arena, TestStruct);

    assert(a != nullptr);
    assert(b != nullptr);
    assert(c != nullptr);

    assert(a != b);
    assert(b != c);
    assert(a != c);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] multiple allocs\n");
}

internal void test_alignment(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    void* p8  = allocator_alloc(&arena, 16, 8);
    void* p16 = allocator_alloc(&arena, 16, 16);
    void* p32 = allocator_alloc(&arena, 16, 32);
    void* p64 = allocator_alloc(&arena, 16, 64);

    assert(p8  != nullptr);
    assert(p16 != nullptr);
    assert(p32 != nullptr);
    assert(p64 != nullptr);

    assert(((uptr)p8  % 8)  == 0);
    assert(((uptr)p16 % 16) == 0);
    assert(((uptr)p32 % 32) == 0);
    assert(((uptr)p64 % 64) == 0);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] alignment\n");
}

internal void test_out_of_memory(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, 128);

    void* first  = allocator_alloc(&arena, 100, 8);
    void* second = allocator_alloc(&arena, 100, 8);

    assert(first != nullptr);
    assert(second == nullptr);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] out of memory\n");
}

internal void test_exact_capacity(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, 128);

    void* first = allocator_alloc(&arena, 128, 1);

    assert(first != nullptr);

    void* second = allocator_alloc(&arena, 1, 1);

    assert(second == nullptr);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] exact capacity\n");
}

internal void test_reset(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    void* first = allocator_alloc(&arena, 128, 8);

    assert(first != nullptr);

    allocator_reset(&arena);

    void* second = allocator_alloc(&arena, 128, 8);

    assert(second != nullptr);
    assert(first == second);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] reset\n");
}

internal void test_free_noop(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    i32* first = ALLOC(&arena, i32);

    allocator_free(&arena, first);

    i32* second = ALLOC(&arena, i32);

    assert(second != nullptr);
    assert(second != first);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] free noop\n");
}

internal void test_realloc_stub(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    i32* value = ALLOC(&arena, i32);

    void* result = allocator_realloc(
        &arena,
        value,
        sizeof(i32),
        sizeof(i64)
    );

    assert(result == nullptr);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] realloc stub\n");
}

internal void test_external_buffer(void) {
    MemorySource source = malloc_memory_source_create();

    u8 buffer[512];

    Allocator arena = arena_create_from_buffer(
        &source,
        buffer,
        sizeof(buffer)
    );

    TestStruct* value = ALLOC(&arena, TestStruct);

    assert(value != nullptr);

    assert((u8*)value >= buffer);
    assert((u8*)value < buffer + sizeof(buffer));

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] external buffer\n");
}

internal void test_external_buffer_reset(void) {
    MemorySource source = malloc_memory_source_create();

    u8 buffer[256];

    Allocator arena = arena_create_from_buffer(
        &source,
        buffer,
        sizeof(buffer)
    );

    void* first = allocator_alloc(&arena, 64, 8);

    allocator_reset(&arena);

    void* second = allocator_alloc(&arena, 64, 8);

    assert(first == second);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] external buffer reset\n");
}

internal void test_source_association(void) {
    MemorySource source = malloc_memory_source_create();

    Allocator arena = arena_create(&source, KB(1));

    assert(arena.source == &source);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    printf("[PASS] source association\n");
}

int main(void) {
    test_create();

    test_alloc_single();
    test_alloc_array();
    test_multiple_allocs();

    test_alignment();

    test_out_of_memory();
    test_exact_capacity();

    test_reset();

    test_free_noop();
    test_realloc_stub();

    test_external_buffer();
    test_external_buffer_reset();

    test_source_association();

    printf("\nAll arena tests passed.\n");

    return 0;
}

