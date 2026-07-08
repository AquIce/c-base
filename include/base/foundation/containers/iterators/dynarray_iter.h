#ifndef __BASE_FOUNDATION_COUNTAINERS_DYNARRAY_ITER__
#define __BASE_FOUNDATION_COUNTAINERS_DYNARRAY_ITER__

#include <base/foundation/macros.h>
#include <base/foundation/containers/dynarray.h>
#include <base/foundation/containers/iterator.h>

#include <assert.h>

extern const IteratorVTable dynarray_iterator_vt;

typedef struct {
    const DynArray *arr;
    usize index;
} DynArrayIterCtx;

internal_fn Iterator dynarray_iter(DynArray* arr) {
	comptime_assert(sizeof(DynArrayIterCtx) <= MAX_ITER_CTX_SIZE);

	Iterator iter = iterator_make(&dynarray_iterator_vt);

	*(DynArrayIterCtx*)iterator_ctx(&iter) = (DynArrayIterCtx){
		.arr = arr,
		.index = 0,
	};

    return iter;
}

#endif // __BASE_FOUNDATION_COUNTAINERS_DYNARRAY_ITER__
