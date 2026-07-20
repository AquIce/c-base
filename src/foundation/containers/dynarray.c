#include <base/foundation/containers/dynarray.h>

#include <base/foundation/macros.h>
#include <base/foundation/containers/container.h>
#include <base/foundation/memory/allocator.h>

#include <assert.h>
#include <string.h>

// --= Local Header =--

internal bool dynarray_ensure_capacity(DynArray*, usize capacity);
internal bool dynarray_push_unchecked(DynArray*, const void* elem);
internal void dynarray_reset_state(DynArray* dynarray);
internal void dynarray_reset_destructive(DynArray* dynarray);

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
		POD_LIFETIME
	);
}
DynArray dynarray_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize elem_size,
	usize alignment,
	const ElementLifetime* elem_lifetime
) {
	void* buffer = nullptr;
	if(capacity != 0) {
		buffer = allocator_alloc(allocator, elem_size * capacity, alignment);
		if(!buffer) {
			return (DynArray){0};
		}
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
    if(dynarray->buffer) {
        dynarray_clear(dynarray);
        allocator_free(dynarray->descriptor.allocator, dynarray->buffer);
    }
    dynarray_reset_state(dynarray);
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
	const ElementLifetime* lifetime = src->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);

	if(lifetime && !lifetime->policy->copy) {
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
	if(lifetime == POD_LIFETIME) {
		(void)memcpy(
			buffer,
			src->buffer,
			src->size * src->descriptor.elem_size
		);
	} else {
		for(usize i = 0; i < src->size; i++) {
			usize offset = i * src->descriptor.elem_size;
			lifetime->policy->copy(
				lifetime->ctx,
				(u8*)buffer + offset,
				(u8*)src->buffer + offset
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
	src->buffer = nullptr;
    src->size = 0;
}


// --= Size =--

internal bool dynarray_ensure_capacity(DynArray* dynarray, usize capacity) {
	usize new_capacity = dynarray->descriptor.capacity;
	while(capacity > new_capacity) {
		new_capacity = new_capacity == 0
			? 1
			: DYNARRAY_GROW_FACTOR * new_capacity;
	}

	return dynarray_reserve(dynarray, new_capacity);
}

bool dynarray_reserve(DynArray* dynarray, usize capacity) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);

	if(capacity <= dynarray->descriptor.capacity) {
		return true;
	}

	if(lifetime && !(lifetime->policy->move || lifetime->policy->copy)) {
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

	if(!dynarray->buffer) {
		dynarray->buffer = buffer;
		dynarray->descriptor.capacity = capacity;
		return true;
	}

	if(!lifetime) {
		(void)memcpy(
			buffer,
			dynarray->buffer,
			dynarray->size * dynarray->descriptor.elem_size
		);
	} else if(lifetime->policy->move) {
		for(usize i = 0; i < dynarray->size; i++) {
			usize offset = i * dynarray->descriptor.elem_size;
			lifetime->policy->move(
				lifetime->ctx,
				(u8*)buffer + offset,
				(u8*)dynarray->buffer + offset
			);
		}
	} else {
		for(usize i = 0; i < dynarray->size; i++) {
			usize offset = i * dynarray->descriptor.elem_size;
			lifetime->policy->copy(
				lifetime->ctx,
				(u8*)buffer + offset,
				(u8*)dynarray->buffer + offset
			);
		}
	}
	allocator_free(dynarray->descriptor.allocator, dynarray->buffer);
	dynarray->buffer = buffer;
	dynarray->descriptor.capacity = capacity;
	return true;
}
bool dynarray_resize(DynArray* dynarray, usize size) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);
	if(lifetime) {
		assert(lifetime->policy->ctor && lifetime->policy->dtor);
	}

	if(size == dynarray->size) {
		return true;
	}
	if(size > dynarray->descriptor.capacity) {
		return false;
	}

	usize old_size = dynarray->size;
	dynarray->size = size;

	// Shrink
	if(size < old_size) {
		if(!lifetime) {
			return true;
		}
		for(usize i = size; i < old_size; i++) {
			usize offset = i * dynarray->descriptor.elem_size;
			lifetime->policy->dtor(
				lifetime->ctx,
				(u8*)dynarray->buffer + offset
			);
		}
		return true;
	}

	// Grow
	if(!dynarray->descriptor.elem_lifetime) {
		return true;
	}
	for(usize i = old_size; i < size; i++) {
		usize offset = i * dynarray->descriptor.elem_size;
		lifetime->policy->ctor(
			lifetime->ctx,
			(u8*)dynarray->buffer + offset
		);
	}
	return true;
}
bool dynarray_shrink_to_fit(DynArray* dynarray) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	if(dynarray->size == dynarray->descriptor.capacity) {
		return true;
	}
	if(dynarray->size == 0) {
		allocator_free(dynarray->descriptor.allocator, dynarray->buffer);
		dynarray->buffer = nullptr;
		dynarray->descriptor.capacity = 0;
		return true;
	}

	void* buffer = allocator_alloc(
		dynarray->descriptor.allocator,
		dynarray->descriptor.elem_size * dynarray->size,
		dynarray->descriptor.alignment
	);
	if(!buffer) {
		return false;
	}

	if(!lifetime) {
		(void)memcpy(
			buffer,
			dynarray->buffer,
			dynarray->size * dynarray->descriptor.elem_size
		);
	} else if(lifetime->policy->move) {
		for(usize i = 0; i < dynarray->size; i++) {
			usize offset = i * dynarray->descriptor.elem_size;
			lifetime->policy->move(
				lifetime->ctx,
				(u8*)buffer + offset,
				(u8*)dynarray->buffer + offset
			);
		}
	} else {
		for(usize i = 0; i < dynarray->size; i++) {
			usize offset = i * dynarray->descriptor.elem_size;
			lifetime->policy->copy(
				lifetime->ctx,
				(u8*)buffer + offset,
				(u8*)dynarray->buffer + offset
			);
		}
	}
	allocator_free(dynarray->descriptor.allocator, dynarray->buffer);
	dynarray->buffer = buffer;
	dynarray->descriptor.capacity = dynarray->size;
	return true;
}


// --= Modifiers =--

internal bool dynarray_push_unchecked(DynArray* dynarray, const void* elem) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	void* dest = (u8*)dynarray->buffer + dynarray->size * dynarray->descriptor.elem_size;

	// POD
	if(!lifetime) {
		(void)memcpy(dest, elem, dynarray->descriptor.elem_size);
		dynarray->size++;
		return true;
	}
	// Move
	if(lifetime->policy->move) {
		lifetime->policy->move(
			lifetime->ctx,
			dest,
			(void*)elem
		);
		dynarray->size++;
		return true;
	}
	// Copy
	if(lifetime->policy->copy) {
		lifetime->policy->copy(
			lifetime->ctx,
			dest,
			elem
		);
		dynarray->size++;
		return true;
	}
	// Non-POD,non-movable, non-copyable
    return false;
}

// WARN: In the case of a movable policy datatype, elem is cast to `void*` and invalidated
bool dynarray_push(DynArray* dynarray, const void* elem) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);

	if(!dynarray_ensure_capacity(dynarray, dynarray->size + 1)) {
		return false;
	}

	return dynarray_push_unchecked(dynarray, elem);
}
// WARN: In the case of a movable policy datatype, elem is cast to `void*` and invalidated
bool dynarray_append(
    DynArray* dynarray,
    const void* data,
    usize count
) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);

	if(count == 0) {
		return true;
	}

    if(!dynarray_ensure_capacity(dynarray, dynarray->size + count)) {
		return false;
	}

	if(!lifetime) {
		(void)memcpy(
			(u8*)dynarray->buffer + dynarray->size * dynarray->descriptor.elem_size,
			data,
			count * dynarray->descriptor.elem_size
		);

		dynarray->size += count;
		return true;
	}

	for(usize i = 0; i < count; i++) {
		void* src_elem = (u8*)data + i * dynarray->descriptor.elem_size;
		if(!dynarray_push_unchecked(dynarray, src_elem)) {
			return false;
		}
	}
	return true;
}

void dynarray_pop(DynArray* dynarray) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);
	if(lifetime) {
		assert(lifetime->policy->dtor);
	}

	if(dynarray->size == 0) { return; }

	if(lifetime) {
		lifetime->policy->dtor(
			lifetime->ctx,
			(u8*)dynarray->buffer + (dynarray->size - 1) * dynarray->descriptor.elem_size
		);
	}

	dynarray->size--;
}

// WARN: In the case of a movable policy datatype, elem is cast to `void*` and invalidated
bool dynarray_insert(
    DynArray* dynarray,
    usize index,
    const void* elem
) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(index <= dynarray->size);

	assert(!lifetime || lifetime->policy);
	if(lifetime) {
		assert(lifetime->policy->dtor);
	}

	if(!dynarray_ensure_capacity(dynarray, dynarray->size + 1)) {
		return false;
	}

	void* elem_addr = (u8*)dynarray->buffer + index * dynarray->descriptor.elem_size;

	if(!lifetime) {
		(void)memmove(
			(u8*)elem_addr + dynarray->descriptor.elem_size,
			elem_addr,
			(dynarray->size - index) * dynarray->descriptor.elem_size
		);
		(void)memcpy(elem_addr, elem, dynarray->descriptor.elem_size);
		dynarray->size++;
		return true;
	}

	if(!lifetime->policy->move && !lifetime->policy->copy) {
		return false;
	}

	for(usize i = dynarray->size; i > index; --i) {
		void* dst = (u8*)dynarray->buffer + i * dynarray->descriptor.elem_size;

		if(lifetime->policy->move) {
			lifetime->policy->move(
				lifetime->ctx,
				dst,
				(u8*)dst - dynarray->descriptor.elem_size
			);
			continue;
		}
		lifetime->policy->copy(
			lifetime->ctx,
			dst,
			(u8*)dst - dynarray->descriptor.elem_size
		);
	}

	if(lifetime->policy->move) {
		lifetime->policy->move(
			lifetime->ctx,
			elem_addr,
			(void*)elem
		);
	} else {
		lifetime->policy->dtor(
			lifetime->ctx,
			elem_addr
		);
		lifetime->policy->copy(
			lifetime->ctx,
			elem_addr,
			elem
		);
	}
	
	dynarray->size++;

	return true;
}

bool dynarray_remove(DynArray* dynarray, usize index) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(index < dynarray->size);

	assert(!lifetime || lifetime->policy);
	if(lifetime) {
		assert(lifetime->policy->dtor);
	}

	void* elem_addr = (u8*)dynarray->buffer + index * dynarray->descriptor.elem_size;

	if(!lifetime) {
		(void)memmove(
			elem_addr,
			(u8*)elem_addr + dynarray->descriptor.elem_size,
			(dynarray->size - index - 1) * dynarray->descriptor.elem_size
		);
		dynarray->size--;
		return true;
	}

	if(!lifetime->policy->move && !lifetime->policy->copy) {
		return false;
	}

	lifetime->policy->dtor(
		lifetime->ctx,
		(u8*)dynarray->buffer + index * dynarray->descriptor.elem_size
	);

	for(usize i = index + 1; i < dynarray->size; i++) {
		void* src = (u8*)dynarray->buffer + i * dynarray->descriptor.elem_size;

		if(lifetime->policy->move) {
			lifetime->policy->move(
				lifetime->ctx,
				(u8*)src - dynarray->descriptor.elem_size,
				src
			);
			continue;
		}
		lifetime->policy->copy(
			lifetime->ctx,
			(u8*)src - dynarray->descriptor.elem_size,
			src
		);
	}
	if(!lifetime->policy->move) {
		lifetime->policy->dtor(
			lifetime->ctx,
			(u8*)dynarray->buffer + (dynarray->size - 1) * dynarray->descriptor.elem_size
		);
	}

	dynarray->size--;

	return true;
}

void dynarray_clear(DynArray* dynarray) {
	const ElementLifetime* lifetime = dynarray->descriptor.elem_lifetime;

	assert(!lifetime || lifetime->policy);
	if(lifetime) {
		assert(lifetime->policy->dtor);
	}

	if(lifetime) {
		for(usize i = 0; i < dynarray->size; i++) {
			lifetime->policy->dtor(
				lifetime->ctx,
				(u8*)dynarray->buffer + i * dynarray->descriptor.elem_size
			);
		}
	}
	dynarray->size = 0;
}

internal void dynarray_reset_state(DynArray* dynarray) {
    dynarray->buffer = nullptr;
    dynarray->size = 0;
    dynarray->descriptor.capacity = 0;
    dynarray->descriptor.elem_size = 0;
    dynarray->descriptor.alignment = 0;
    dynarray->descriptor.allocator = nullptr;
    dynarray->descriptor.elem_lifetime = nullptr;
}
internal void dynarray_reset_destructive(DynArray* dynarray) {
    dynarray_clear(dynarray);
    dynarray_reset_state(dynarray);
}
