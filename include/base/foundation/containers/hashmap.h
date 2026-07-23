#ifndef __BASE_FOUNDATION_CONTAINERS_HASHMAP__
#define __BASE_FOUNDATION_CONTAINERS_HASHMAP__

#include <base/foundation/memory/allocator.h>
#include <base/foundation/containers/container.h>

#define HASHMAP_CREATE(T, allocator, hashFunc, capacity) \
	hashmap_create((allocator), (hashFunc), (capacity), sizeof(T), alignof(T))

#define HASHMAP_CREATE_COMPLEX(T, allocator, hashFunc, capacity, policy) \
	hashmap_create_complex((allocator), (hashFunc), (capacity), sizeof(T), alignof(T), (policy))

#define HASHMAP_AT(arr, T, key) \
    ({ \
        __auto_type tmp = (key); \
        (T*)hashmap_at((arr), (void*)&tmp); \
    })

#define HASHMAP_INSERT(arr, key, value) \
    ({ \
        __auto_type tmp_key = (key); \
        __auto_type tmp_value = (value); \
        hashmap_insert((arr), ((void*)&tmp_key), (void*)&tmp_value); \
    })

#define HASHMAP_GROW_FACTOR 2

typedef struct {
	usize capacity;

	usize key_size;
	usize elem_size;
	usize alignment;

	const Allocator* allocator;

	const ElementLifetime* key_lifetime;
	const ElementLifetime* elem_lifetime;
} HashMapDescriptor;

typedef struct {
	void* buffer;
	void* meta_buffer;
	usize elem_count;

	HashMapDescriptor descriptor;
} HashMap;


// --= Element Lifetime =--

extern const ElementPolicy HASHMAP_ELEMENT_POLICY;


// --= Creation / Destruction =--

HashMap hashmap_create(
	const Allocator* allocator,
	usize capacity,
	usize key_size,
	usize elem_size,
	usize alignment,
	const ElementLifetime* key_lifetime
);
HashMap hashmap_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize key_size,
	usize elem_size,
	usize alignment,
	const ElementLifetime* key_policy,
	const ElementLifetime* elem_policy
);
void hashmap_destroy(HashMap*);

bool hashmap_copy(
    HashMap* dest,
    const HashMap* src
);
bool hashmap_copy_walloc(
    HashMap* dest,
    const HashMap* src,
	const Allocator* allocator
);
void hashmap_move(
    HashMap* dest,
    HashMap* src
);


// --= Size =--

internal_fn usize hashmap_size(const HashMap* hashmap) {
	return hashmap->elem_count;
}
internal_fn usize hashmap_capacity(const HashMap* hashmap) {
	return hashmap->descriptor.capacity;
}

internal_fn bool hashmap_empty(const HashMap* hashmap) {
	return hashmap->elem_count == 0;
}

bool hashmap_reserve(HashMap*, usize capacity);


// --= Element Access =--

internal_fn void* hashmap_at(HashMap* hashmap, const void* key) {
	usize hash = hashmap->descriptor.key_lifetime->policy->hash(
		hashmap->descriptor.key_lifetime->ctx,
		key
	);
	return (void*)((u8*)hashmap->buffer + hash * hashmap->descriptor.elem_size);
}
internal_fn const void* hashmap_at_const(const HashMap* hashmap, const void* key) {
	usize hash = hashmap->descriptor.key_lifetime->policy->hash(
		hashmap->descriptor.key_lifetime->ctx,
		key
	);
	return (const void*)((u8*)hashmap->buffer + hash * hashmap->descriptor.elem_size);
}


// --= Modifiers =--

bool hashmap_insert(HashMap*, const void* key, const void* elem);

bool hashmap_remove(HashMap*, const void* key);

void hashmap_clear(HashMap*);


#endif // __BASE_FOUNDATION_CONTAINERS_HASHMAP__
