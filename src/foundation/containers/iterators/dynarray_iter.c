#include <base/foundation/containers/iterators/dynarray_iter.h>

#include <base/foundation/containers/iterator.h>

// --= Local Header =--

internal bool dynarray_iter_next(Iterator* iter, void* out);
internal void dynarray_iter_destroy(Iterator* iter);

const IteratorVTable dynarray_iterator_vt = {
    .next = &dynarray_iter_next,
	.destroy = &dynarray_iter_destroy,
};

// --= Implementation =--

internal bool dynarray_iter_next(Iterator* iter, void* out) {
    DynArrayIterCtx* ctx = (DynArrayIterCtx*)iterator_ctx(iter);

    if(ctx->index >= ctx->arr->size) {
        return false;
    }

    *(void**)out = (u8*)ctx->arr->buffer + ctx->index * ctx->arr->descriptor.elem_size;
    ctx->index++;

    return true;
}

internal void dynarray_iter_destroy(Iterator* iter) {
	(void)iter;
}
