#ifndef __BASE_FOUNDATION_CONTAINERS_ITERATORS_ADAPTERS_MAP_ITER__
#define __BASE_FOUNDATION_CONTAINERS_ITERATORS_ADAPTERS_MAP_ITER__

#include <base/foundation/macros.h>
#include <base/foundation/containers/iterator.h>
#include <base/foundation/memory/allocator.h>

extern const IteratorVTable map_iterator_vt;

typedef void (*IteratorMapFn)(const void* in, void* out, void* ctx);

typedef struct {
	const Allocator* alloc;
    void* fn_ctx;

	IteratorMapFn map_fn;

    Iterator src;

	alignas(MAX_ALIGNMENT) u8 current[MAX_ITER_VALUE_SIZE];
} MapIterCtx;

internal_fn Iterator map_iter(const Allocator* alloc, Iterator src, IteratorMapFn map_fn, void* ctx) {

	MapIterCtx* map = ALLOC(alloc, MapIterCtx);
    *map = (MapIterCtx){
        .src = src,
        .map_fn = map_fn,
        .fn_ctx = ctx,
		.alloc = alloc,
    };

	Iterator iter = iterator_make_ex(
        &map_iterator_vt,
        map
    );

	return iter;
}

#endif // __BASE_FOUNDATION_CONTAINERS_ITERATORS_ADAPTERS_MAP_ITER__
