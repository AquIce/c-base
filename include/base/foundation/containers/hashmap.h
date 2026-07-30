#ifndef __BASE_FOUNDATION_CONTAINERS_HASHMAP__
#define __BASE_FOUNDATION_CONTAINERS_HASHMAP__

#include <base/foundation/memory/allocator.h>
#include <base/foundation/containers/container.h>

#define HASHMAP_CREATE(K, V, allocator, capacity, key_lifetime) \
	hashmap_create( \
		(allocator), \
		(capacity), \
		sizeof(K), \
		sizeof(V), \
		alignof(K), \
		alignof(V), \
		(key_lifetime) \
	)

#define HASHMAP_CREATE_COMPLEX(K, V, allocator, capacity, key_lifetime, elem_lifetime) \
	hashmap_create_complex( \
		(allocator), \
		(capacity), \
		sizeof(K), \
		sizeof(V), \
		alignof(K), \
		alignof(V), \
		(key_lifetime) \
		(elem_lifetime) \
	)

#define HASHMAP_AT(hashmap, T, key) \
    ({ \
        __auto_type tmp = (key); \
        (T*)hashmap_at((hashmap), (void*)&tmp); \
    })

#define HASHMAP_INSERT(hashmap, key, value) \
    ({ \
        __auto_type tmp_key = (key); \
        __auto_type tmp_value = (value); \
        hashmap_insert((hashmap), ((void*)&tmp_key), (void*)&tmp_value); \
    })

#define HASHMAP_GROW_FACTOR 2

typedef struct {
	usize capacity;

	usize key_size;
	usize elem_size;
	usize key_alignment;
	usize elem_alignment;

	const Allocator* allocator;

	// NOTE: Key lifetime is mandatory (for the `hash`).
	// If it has a `copy`, it will be copied, otherwise, it is copied as a POD.
	const ElementLifetime* key_lifetime;
	// NOTE :Element lifetime is optional.
	// If it has a `move`, it will be moved, if if it has a `copy`, it will be copied, otherwise it is copied/moved as a POD.
	const ElementLifetime* elem_lifetime;
} HashMapDescriptor;

typedef struct {
	void* buffer;
	void* keys_buffer;
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
	usize key_alignment,
	usize elem_alignment,
	const ElementLifetime* key_lifetime
);
HashMap hashmap_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize key_size,
	usize elem_size,
	usize key_alignment,
	usize elem_alignment,
	const ElementLifetime* key_lifetime,
	const ElementLifetime* elem_lifetime
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

bool hashmap_grow(HashMap*, usize new_capacity);


// --= Element Access =--

void* hashmap_at(const HashMap* hashmap, const void* key);
const void* hashmap_at_const(const HashMap* hashmap, const void* key);

bool hashmap_has(const HashMap* hashmap, const void* key);

// --= Modifiers =--

bool hashmap_insert(HashMap*, const void* key, void* elem);

bool hashmap_remove(HashMap*, const void* key);

void hashmap_clear(HashMap*);


#endif // __BASE_FOUNDATION_CONTAINERS_HASHMAP__
