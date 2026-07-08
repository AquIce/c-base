#include <base/foundation/containers/iterators/adapters/map_iter.h>

#include <base/foundation/containers/iterator.h>

// --= Local Header =--

internal bool map_iter_next(Iterator* iter, void* out);
internal void map_iter_destroy(Iterator* iter);

const IteratorVTable map_iterator_vt = {
    .next = &map_iter_next,
    .destroy = &map_iter_destroy,
};

// --= Implementation =--

internal bool map_iter_next(Iterator* iter, void* out) {
    MapIterCtx* ctx = (MapIterCtx*)iterator_ctx(iter);

	void* input;
    if(!iterator_next(&ctx->src, &input)) {
        return false;
    }

    ctx->map_fn(input, ctx->current, ctx->fn_ctx);

	*(void**)out = ctx->current;

    return true;
}

internal void map_iter_destroy(Iterator* iter) {
    MapIterCtx* ctx = (MapIterCtx*)iterator_ctx(iter);

    iterator_destroy(&ctx->src);

    allocator_free(
        ctx->alloc,
        ctx
    );
}
