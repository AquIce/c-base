#include <base/foundation/macros.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/containers/dynarray.h>

#include <stdio.h>

void print_dynarray(const DynArray* arr) {
	for(usize i = 0; i < dynarray_size(arr); i++) {
        printf("\t[%zu]: %d\n", i, *DYNARRAY_AT(arr, i32, i));
    }
}

i32 main(void) {
    MemorySource source = malloc_memory_source_create();
    Allocator arena = arena_create(&source, KB(4));

    DynArray numbers = DYNARRAY_CREATE(i32, &arena, 4);

    DYNARRAY_PUSH(&numbers, 10);
    DYNARRAY_PUSH(&numbers, 20);
    DYNARRAY_PUSH(&numbers, 30);

    printf("After triple push:\n");
    print_dynarray(&numbers);

    DYNARRAY_INSERT(&numbers, 1, 15);

    printf("After insert:\n");
    print_dynarray(&numbers);

    dynarray_remove(&numbers, 2);

    printf("\nAfter remove:\n");
	print_dynarray(&numbers);

    dynarray_pop(&numbers);

    printf("\nSize after pop: %zu\n", dynarray_size(&numbers));

    dynarray_clear(&numbers);

    printf("Empty: %s\n", dynarray_empty(&numbers) ? "true" : "false");

    dynarray_destroy(&numbers);

    arena_destroy(&arena);
    malloc_memory_source_destroy(&source);

    return 0;
}

