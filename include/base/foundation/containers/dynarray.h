#ifndef __BASE_FOUNDATION_CONTAINERS_DYNARRAY__
#define __BASE_FOUNDATION_CONTAINERS_DYNARRAY__

#include "container.h"
#include <base/foundation/memory/allocator.h>
#include <base/foundation/containers/container.h>

#define DYNARRAY_CREATE(T, allocator, capacity) \
	dynarray_create((allocator), (capacity), sizeof(T), alignof(T))

#define DYNARRAY_CREATE_COMPLEX(T, allocator, capacity, policy) \
	dynarray_create_complex((allocator), (capacity), sizeof(T), alignof(T), (policy))

#define DYNARRAY_AT(arr, T, i) \
    ((T*)dynarray_at_const((arr), (i)))

#define DYNARRAY_PUSH(arr, value) \
    ({ \
        __auto_type tmp = (value); \
        dynarray_push((arr), &tmp); \
    })

#define DYNARRAY_INSERT(arr, index, value) \
    ({ \
        __auto_type tmp = (value); \
        dynarray_insert((arr), &tmp, (index)); \
    })

#define DYNARRAY_GROW_FACTOR 2

typedef struct {
	usize capacity;

	usize elem_size;
	usize alignment;

	const Allocator* allocator;

	const ElementLifetime* elem_lifetime;
} DynArrayDescriptor;

typedef struct {
	void* buffer;
	usize size;

	DynArrayDescriptor descriptor;
} DynArray;


// --= Element Lifetime =--

extern const ElementPolicy DYNARRAY_ELEMENT_POLICY;

DynArrayDescriptor dynarray_ctx_make(
    const Allocator* allocator,
    size_t capacity,
    size_t elem_size,
    size_t alignment
);
DynArrayDescriptor dynarray_ctx_make_complex(
    const Allocator* allocator,
    size_t capacity,
    size_t elem_size,
    size_t alignment,
    const ElementLifetime* lifetime
);


// --= Creation / Destruction =--

DynArray dynarray_create(
	const Allocator* allocator,
	usize capacity,
	usize elem_size,
	usize alignment
);
DynArray dynarray_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize elem_size,
	usize alignment,
	const ElementLifetime* elem_policy
);
void dynarray_destroy(DynArray*);

bool dynarray_copy(
    DynArray* dest,
    const DynArray* src
);
bool dynarray_copy_walloc(
    DynArray* dest,
    const DynArray* src,
	const Allocator* allocator
);
void dynarray_move(
    DynArray* dest,
    DynArray* src
);


// --= Size =--

internal_fn usize dynarray_size(const DynArray* dynarray) {
	return dynarray->size;
}
internal_fn usize dynarray_capacity(const DynArray* dynarray) {
	return dynarray->descriptor.capacity;
}

internal_fn bool dynarray_empty(const DynArray* dynarray) {
	return dynarray->size == 0;
}

bool dynarray_reserve(DynArray*, usize capacity);
bool dynarray_resize(DynArray*, usize size);
bool dynarray_shrink_to_fit(DynArray*);

// --= Element Access =--

internal_fn void* dynarray_at(DynArray* dynarray, usize index) {
	assert(0 <= index);
	assert(index < dynarray->size);
	return (char*)dynarray->buffer + index * dynarray->descriptor.elem_size;
}
internal_fn const void* dynarray_at_const(const DynArray* dynarray, usize index) {
	assert(0 <= index);
    assert(index < dynarray->size);
    return (const char*)dynarray->buffer + index * dynarray->descriptor.elem_size;
}

internal_fn void* dynarray_front(DynArray* dynarray) {
	assert(dynarray->size > 0);
	return dynarray->buffer;
}
internal_fn void* dynarray_back(DynArray* dynarray) {
	assert(dynarray->size > 0);
	return (char*)dynarray->buffer + (dynarray->size - 1) * dynarray->descriptor.elem_size;
}

internal_fn void* dynarray_data(DynArray* dynarray) {
	return dynarray->buffer;
}
internal_fn const void* dynarray_data_const(const DynArray* dynarray) {
	return dynarray->buffer;
}

// --= Modifiers =--

bool dynarray_push(DynArray*, const void* elem);
bool dynarray_append(
	DynArray*,
	const void* data,
	usize size
);

void dynarray_pop(DynArray*);

bool dynarray_insert(
    DynArray*,
    usize index,
    const void* elem
);

bool dynarray_remove(DynArray*, usize index);

void dynarray_clear(DynArray*);


#endif // __BASE_FOUNDATION_CONTAINERS_DYNARRAY__
