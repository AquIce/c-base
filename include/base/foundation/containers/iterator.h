#ifndef __BASE_FOUNDATION_CONTAINERS_ITERATOR__
#define __BASE_FOUNDATION_CONTAINERS_ITERATOR__

#include <base/foundation/macros.h>

#define MAX_ITER_CTX_SIZE 32
#define MAX_ITER_VALUE_SIZE 32

typedef struct Iterator Iterator;

typedef bool (*IterNextFn)(Iterator* iter, void* out);
typedef void (*IterDestroyFn)(Iterator* iter);

typedef struct {
	IterNextFn next;
	IterDestroyFn destroy;
} IteratorVTable;

struct Iterator {
	const IteratorVTable* vt;

	// NOTE: For iterators with small ctx, `uses_external_storage = false`, and use `inline_storage`
	// NOTE: For iterators with bigger ctx, `uses_external_storage = true`, and `external_storage` point to whatever storage
	bool uses_external_storage;
	union {
		void* external_storage;
		alignas(MAX_ALIGNMENT) u8 inline_storage[MAX_ITER_CTX_SIZE];
	};
};

internal_fn Iterator iterator_make(const IteratorVTable* vt) {
    Iterator it = {
        .vt = vt,
		.uses_external_storage = false,
    };

    return it;
}

internal_fn Iterator iterator_make_ex(const IteratorVTable* vt, void* ctx) {
    Iterator it = {
        .vt = vt,
		.uses_external_storage = true,
		.external_storage = ctx,
    };

    return it;
}

internal_fn void* iterator_ctx(Iterator* it) {
    return it->uses_external_storage
		? it->external_storage
        : it->inline_storage;
}

// NOTE: `out` must point to storage capable of holding an element pointer.
// Example:
//     Foo* foo;
//     iterator_next(&iter, &foo);
internal_fn bool iterator_next(Iterator* it, void* out) {
    return it->vt->next(it, out);
}

internal_fn void iterator_destroy(Iterator* it) {
    if(it->vt->destroy) {
        it->vt->destroy(it);
    }
}

#endif // __BASE_FOUNDATION_CONTAINERS_ITERATOR__
