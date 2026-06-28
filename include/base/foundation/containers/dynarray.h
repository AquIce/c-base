#ifndef __BASE_FOUNDATION_CONTAINERS_DYNARRAY__
#define __BASE_FOUNDATION_CONTAINERS_DYNARRAY__

#include "container.h"
#include <base/foundation/memory/allocator.h>
#include <base/foundation/containers/container.h>

#define DYNARRAY_CREATE(T, allocator, capacity) \
	dynarray_create((allocator), (capacity), sizeof(T), alignof(T))

#define DYNARRAY_CREATE_COMPLEX(T, allocator, capacity, policy) \
	dynarray_create((allocator), (capacity), sizeof(T), alignof(T), (policy))


#define DYNARRAY_PUSH(dynarray, value) \
	do { \
        __auto_type tmp = (value); \
        dynarray_push((arr), &tmp); \
    } while(0)

#define DYNARRAY_INSERT(dynarray, value, index) \
	do { \
        __auto_type tmp = (value); \
		dynarray_insert((arr), &tmp, (index)); \
    } while(0)

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

usize dynarray_size(const DynArray*);
usize dynarray_capacity(const DynArray*);

bool dynarray_empty(const DynArray*);

bool dynarray_reserve(DynArray*, usize capacity);
bool dynarray_resize(DynArray*, usize size);
bool dynarray_shrink_to_fit(DynArray*);


// --= Element Access =--

void* dynarray_at(DynArray*, usize index);
const void* dynarray_at_const(const DynArray*, usize index);

void* dynarray_front(DynArray*);
void* dynarray_back(DynArray*);

void* dynarray_data(DynArray*);
const void* dynarray_data_const(const DynArray*);


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

void dynarray_remove(DynArray*, usize index);

void dynarray_clear(DynArray*);
void dynarray_reset(DynArray*);


#endif // __BASE_FOUNDATION_CONTAINERS_DYNARRAY__
