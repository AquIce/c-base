#include <base/foundation/containers/dynarray.h>
#include <base/foundation/containers/iterators/dynarray_iter.h>
#include <base/foundation/containers/iterators/adapters/map_iter.h>
#include <base/foundation/memory/arena.h>
#include <base/foundation/memory/memory.h>
#include <base/foundation/memory/stack.h>

#include <stdio.h>

internal void int_to_float(const void* in, void* out, void* ctx) {
    (void)ctx;

	int value = *(const int*)in;
    float* result = out;

    *result = (float)value * 1.5f;
}

internal void square(const void* in, void* out, void* ctx) {
    (void)ctx;

	float value = *(const float*)in;
    float* result = out;

    *result = value * value;
}

int main(void) {
	MemorySource source = malloc_memory_source_create();
	Allocator arena = arena_create(&source, KB(4));

	DynArray numbers = DYNARRAY_CREATE(int, &arena, 0);

    for (int i = 1; i <= 5; i++) {
        dynarray_push(&numbers, &i);
    }

	Iterator iter = map_iter(
		&arena,
		map_iter(
			&arena,
			dynarray_iter(&numbers),
			int_to_float,
			nullptr
		),
		square,
		nullptr
	);

    float* value;

	while(iterator_next(&iter, &value)) {
        printf("%.2f\n", *value);
    }
	// Technically does nothing because arena_free is NOOP
    // iterator_destroy(&iter);
	// iterator_destroy(&otherIter);

    dynarray_destroy(&numbers);
    arena_destroy(&arena);
	malloc_memory_source_destroy(&source);

    return 0;
}
