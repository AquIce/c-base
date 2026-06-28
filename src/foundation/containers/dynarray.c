#include <base/foundation/containers/dynarray.h>

#include <base/foundation/macros.h>
#include <base/foundation/containers/container.h>
#include <base/foundation/memory/allocator.h>

#include <assert.h>

// --= Element Lifetime =--

internal void dynarray_policy_ctor(void* ctx, void* elem) {
	if(!ctx || !elem) { return; }
	DynArrayDescriptor* descriptor = (DynArrayDescriptor*)ctx;
	*(DynArray*)elem = dynarray_create_complex(
		descriptor->allocator,
		descriptor->capacity,
		descriptor->elem_size,
		descriptor->alignment,
		descriptor->elem_lifetime
	);
}
internal void dynarray_policy_dtor(void* ctx, void* elem) {
	if(!elem) { return; }
	(void)ctx;
	dynarray_destroy((DynArray*)elem);
}

const ElementPolicy DYNARRAY_ELEMENT_POLICY = (ElementPolicy){
	.ctor = &dynarray_policy_ctor,
	.dtor = &dynarray_policy_dtor,
};

DynArrayDescriptor dynarray_ctx_make(
    const Allocator* allocator,
    size_t capacity,
    size_t elem_size,
    size_t alignment
) {
	return dynarray_ctx_make_complex(
		allocator,
		capacity,
		elem_size,
		alignment,
		POD_LIFETIME
	);
}
DynArrayDescriptor dynarray_ctx_make_complex(
    const Allocator* allocator,
    size_t capacity,
    size_t elem_size,
    size_t alignment,
    const ElementLifetime* lifetime
) {
	return (DynArrayDescriptor){
		.capacity = capacity,
		.elem_size = elem_size,
		.alignment = alignment,
		.allocator = allocator,
		.elem_lifetime = lifetime
	};
}


// --= Creation / Destruction =--

DynArray dynarray_create(
	const Allocator* allocator,
	usize capacity,
	usize elem_size,
	usize alignment
) {
	return dynarray_create_complex(
		allocator,
		capacity,
		elem_size,
		alignment,
		nullptr
	);
}
DynArray dynarray_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize elem_size,
	usize alignment,
	const ElementLifetime* elem_lifetime
) {
	void* buffer = allocator_alloc(allocator, elem_size * capacity, alignment);
	if(!buffer) {
		return (DynArray){0};
	}
	return (DynArray){
		.buffer = buffer,
		.size = 0,
		.descriptor = (DynArrayDescriptor){
			.capacity = capacity,
			.elem_size = elem_size,
			.alignment = alignment,
			.allocator = allocator,
			.elem_lifetime = elem_lifetime,
		},
	};
}


void dynarray_destroy(DynArray* dynarray) {
	allocator_free(dynarray->descriptor.allocator, dynarray->buffer);
	dynarray->buffer = nullptr;
	dynarray->size = 0;
	dynarray->descriptor.capacity = 0;
	dynarray->descriptor.elem_size = 0;
	dynarray->descriptor.alignment = 0;
	dynarray->descriptor.allocator = nullptr;
	dynarray->descriptor.elem_lifetime = nullptr;
}


// NOTE: Copies every element from src to dest
// This can fall into one of the following cases
// - POD type				-> copied using `memset`
// - Non-copyable type		-> function aborts and returns `false`
// - Policy-copyable type	-> copied using policy's copy function
bool dynarray_copy(
    DynArray* dest,
    const DynArray* src
) {
	return dynarray_copy_walloc(dest, src, src->descriptor.allocator);
}
bool dynarray_copy_walloc(
    DynArray* dest,
    const DynArray* src,
	const Allocator* allocator
) {
	assert(!src->descriptor.elem_lifetime || src->descriptor.elem_lifetime->policy);
	if(
		src->descriptor.elem_lifetime
		&& !src->descriptor.elem_lifetime->policy->copy
	) {
		return false;
	}

	void* buffer = allocator_alloc(
		allocator,
		src->descriptor.elem_size * src->descriptor.capacity,
		src->descriptor.alignment
	);
	if(!buffer) {
		return false;
	}	
	if(src->descriptor.elem_lifetime == POD_LIFETIME) {
		memcpy(
			buffer,
			src->buffer,
			src->size * src->descriptor.elem_size
		);
	} else {
		for(usize i = 0; i < src->size; i++) {
			usize offset = i * src->descriptor.elem_size;
			src->descriptor.elem_lifetime->policy->copy(
				src->descriptor.elem_lifetime->ctx,
				(char*)buffer + offset,
				(char*)src->buffer + offset
			);
		}
	}
	dest->buffer = buffer;
	dest->size = src->size;
	dest->descriptor = src->descriptor;
	dest->descriptor.allocator = allocator;
	return true;
}
// NOTE: Moves the whole array, not the elements (ownership transfer)
// WARN: Empties `src`
void dynarray_move(
	DynArray* dest,
	DynArray* src
) {
	dynarray_destroy(dest);
	*dest = *src;
	dynarray_reset(src);
}


// --= Size =--

usize dynarray_size(const DynArray* dynarray) {
	return dynarray->size;
}
usize dynarray_capacity(const DynArray* dynarray) {
	return dynarray->descriptor.capacity;
}

bool dynarray_empty(const DynArray* dynarray) {
	return dynarray->size == 0;
}

bool dynarray_reserve(DynArray* dynarray, usize capacity) {
	assert(!dynarray->descriptor.elem_lifetime || dynarray->descriptor.elem_lifetime->policy);

	if(capacity <= dynarray->descriptor.capacity) {
		return true;
	}

	if(dynarray->descriptor.elem_lifetime && !dynarray->descriptor.elem_lifetime->policy->move) {
		return false;
	}

	void* buffer = allocator_alloc(
		dynarray->descriptor.allocator,
		dynarray->descriptor.elem_size * capacity,
		dynarray->descriptor.alignment
	);
	if(!buffer) {
		return false;
	}
	if(!dynarray->descriptor.elem_lifetime) {
		memcpy(
			buffer,
			dynarray->buffer,
			dynarray->size * dynarray->descriptor.elem_size
		);
	} else {
		for(usize i = 0; i < dynarray->size; i++) {
			usize offset = i * dynarray->descriptor.elem_size;
			dynarray->descriptor.elem_lifetime->policy->move(
				dynarray->descriptor.elem_lifetime->ctx,
				(char*)buffer + offset,
				(char*)dynarray->buffer + offset
			);
		}
	}
	allocator_free(dynarray->descriptor.allocator, dynarray->buffer);
	dynarray->buffer = buffer;
	dynarray->descriptor.capacity = capacity;
	return true;
}
bool dynarray_resize(DynArray* dynarray, usize size) {
	TODO("Add resize using ctor on elements.");
	if(size == dynarray->size) {
		return true;
	}
	if(size > dynarray->descriptor.capacity) {
		return false;
	}

	usize old_size = dynarray->size;
	dynarray->size = size;
	if(size < old_size) {
		return true;
	}
}
bool dynarray_shrink_to_fit(DynArray* dynarray) {
	TODO_IMPL()
}


// --= Element Access =--

void* dynarray_at(DynArray* dynarray, usize index) {
	TODO_IMPL()
}
const void* dynarray_at_const(const DynArray* dynarray, usize index) {
	TODO_IMPL()
}

void* dynarray_front(DynArray* dynarray) {
	TODO_IMPL()
}
void* dynarray_back(DynArray* dynarray) {
	TODO_IMPL()
}

void* dynarray_data(DynArray* dynarray) {
	TODO_IMPL()
}
const void* dynarray_data_const(const DynArray* dynarray) {
	TODO_IMPL()
}


// --= Modifiers =--

bool dynarray_push(DynArray* dynarray, const void* elem) {
	TODO_IMPL()
}
bool dynarray_append(
	DynArray* dynarray,
	const void* data,
	usize size
) {
	TODO_IMPL()
}

void dynarray_pop(DynArray* dynarray) {
	TODO_IMPL()
}

bool dynarray_insert(
    DynArray* dynarray,
    usize index,
    const void* elem
) {
	TODO_IMPL()
}

void dynarray_remove(DynArray* dynarray, usize index) {
	TODO_IMPL()
}

void dynarray_clear(DynArray* dynarray) {
	TODO_IMPL()
}
void dynarray_reset(DynArray* dynarray) {
	TODO_IMPL()
}

