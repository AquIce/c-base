#include <base/foundation/macros.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

#include <stdio.h>

i32 main(void) {
    MemorySource source = malloc_memory_source_create();
    Allocator arena = arena_create(&source, KB(1));

    i32* a = ALLOC(&arena, i32);
    i32* b = ALLOC(&arena, i32);
    i32* c = ALLOC(&arena, i32);

    *a = 10;
    *b = 20;
    *c = 30;

    printf("%d %d %d\n", *a, *b, *c);

    i32* arr = NALLOC(&arena, i32, 5);

    for(i32 i = 0; i < 5; i++) {
        arr[i] = (i + 1) * 11;
    }

    for(i32 i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    allocator_reset(&arena);

    i32* d = ALLOC(&arena, i32);
    *d = 99;

    printf("%d\n", *d);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    return 0;
}
