#include <base/foundation/macros.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/allocator.h>

#include <stdio.h>

int main(void) {
    MemorySource source = malloc_memory_source_create();
    Allocator arena = arena_create(&source, KB(1));

    int* a = ALLOC(&arena, int);
    int* b = ALLOC(&arena, int);
    int* c = ALLOC(&arena, int);

    *a = 10;
    *b = 20;
    *c = 30;

    printf("%d %d %d\n", *a, *b, *c);

    int* arr = NALLOC(&arena, int, 5);

    for(int i = 0; i < 5; i++) {
        arr[i] = (i + 1) * 11;
    }

    for(int i = 0; i < 5; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");

    allocator_reset(&arena);

    int* d = ALLOC(&arena, int);
    *d = 99;

    printf("%d\n", *d);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    return 0;
}
