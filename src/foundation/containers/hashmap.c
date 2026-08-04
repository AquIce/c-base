#include <base/foundation/containers/hashmap.h>

#include <base/foundation/macros.h>
#include <base/foundation/containers/container.h>
#include <base/foundation/memory/allocator.h>

#include <assert.h>
#include <string.h>

// --= Local Header =--

internal const hash_t INVALID_INDEX = (hash_t)(-1);

typedef enum : u8 {
    HASHMAP_SLOT_EMPTY     = 0,
    HASHMAP_SLOT_OCCUPIED  = 1,
    HASHMAP_SLOT_TOMBSTONE = 2,
} HashMapSlotState;

internal_fn hash_t hashmap_freestanding_hash(const HashMap* hashmap, const void* key, usize capacity) {
	return hashmap->descriptor.key_lifetime->policy->hash(
		hashmap->descriptor.key_lifetime->ctx, key
	) % capacity;
}
internal_fn hash_t hashmap_hash(const HashMap* hashmap, const void* key) {
	return hashmap->descriptor.key_lifetime->policy->hash(
		hashmap->descriptor.key_lifetime->ctx, key
	) % hashmap->descriptor.capacity;
}

internal_fn HashMapSlotState hashmap_get_slot_state(const HashMap* hashmap, hash_t index) {
	return *((HashMapSlotState*)hashmap->meta_buffer + index);
}
internal_fn void hashmap_set_slot_state(const HashMap* hashmap, hash_t index, HashMapSlotState state) {
	*((HashMapSlotState*)hashmap->meta_buffer + index) = state;
}
internal_fn void* hashmap_get_key(const HashMap* hashmap, hash_t index) {
	return (
		(u8*)hashmap->keys_buffer + index * hashmap->descriptor.key_size
	);
}
internal_fn void* hashmap_get_elem(const HashMap* hashmap, hash_t index) {
	return (
		(u8*)hashmap->buffer + index * hashmap->descriptor.elem_size
	);
}

internal void hashmap_relocate_buffers(
	const HashMap*,
	void* dest_buffer,
	void* dest_keys_buffer,
	void* dest_meta_buffer,
	const void* src_buffer,
	const void* src_keys_buffer,
	const void* src_meta_buffer,
	usize capacity
);

internal void hashmap_rehash(
	void* dest_buffer,
	void* dest_keys_buffer,
	void* dest_meta_buffer,
	usize new_capacity,
	const HashMap* src_hashmap
);


internal bool hashmap_insert_any(
	HashMap*,
	void* key,
	void* elem,
	bool force_move
);
internal void hashmap_insert_at_location(
	const HashMap*,
	void* dest_elem,
	const void* src_elem,
	void* dest_key,
	const void* src_key,
	bool force_move
);
internal void hashmap_insert_at_slot(HashMap*, hash_t index, const void* key, const void* elem, bool force_move);
internal void hashmap_remove_at_slot(HashMap*, hash_t index);

internal void hashmap_reset_state(HashMap*);
internal void hashmap_reset_destructive(HashMap*);


// --= Element Lifetime =--

internal void hashmap_policy_ctor(void* ctx, void* elem) {
	if(!ctx || !elem) { return; }
	HashMapDescriptor* descriptor = (HashMapDescriptor*)ctx;
	*(HashMap*)elem = hashmap_create_complex(
		descriptor->allocator,
		descriptor->capacity,
		descriptor->key_size,
		descriptor->elem_size,
		descriptor->key_alignment,
		descriptor->elem_alignment,
		descriptor->hash_strategy,
		descriptor->key_lifetime,
		descriptor->elem_lifetime
	);
}
internal void hashmap_policy_dtor(void* ctx, void* elem) {
	if(!elem) { return; }
	(void)ctx;
	hashmap_destroy((HashMap*)elem);
}
internal void hashmap_policy_copy(void* ctx, void* dest, const void* src) {
	TODO_IMPL();
}
internal void hashmap_policy_move(void* ctx, void* dest, void* src) {
	TODO_IMPL();
}
internal bool hashmap_policy_equals(void* ctx, const void* elem, const void* other) {
	TODO_IMPL();
}
internal hash_t hashmap_policy_hash(void* ctx, const void* object) {
	TODO_IMPL();
}

// TODO: Add
// - `copy`
// - `move`
// - `equals`
// - `hash`

const ElementPolicy HASHMAP_ELEMENT_POLICY = (ElementPolicy){
	.ctor = &hashmap_policy_ctor,
	.dtor = &hashmap_policy_dtor,
	.copy = &hashmap_policy_copy,
	.move = &hashmap_policy_move,
	.equals = &hashmap_policy_equals,
	// NOTE: HashMaps are not comparable
	.compare = nullptr,
	.hash = &hashmap_policy_hash,
};


// --= Creation / Destruction =--

HashMap hashmap_create(
	const Allocator* allocator,
	usize capacity,
	usize key_size,
	usize elem_size,
	usize key_alignment,
	usize elem_alignment,
	HashMapHashStrategy hash_strategy,
	const ElementLifetime* key_lifetime
) {
	return hashmap_create_complex(
		allocator,
		capacity,
		key_size,
		elem_size,
		key_alignment,
		elem_alignment,
		hash_strategy,
		key_lifetime,
		POD_LIFETIME
	);
}
HashMap hashmap_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize key_size,
	usize elem_size,
	usize key_alignment,
	usize elem_alignment,
	HashMapHashStrategy hash_strategy,
	const ElementLifetime* key_lifetime,
	const ElementLifetime* elem_lifetime
) {
	assert(key_lifetime && key_lifetime->policy);
	assert(!elem_lifetime || elem_lifetime->policy);

	assert(!elem_lifetime || elem_lifetime->policy->move || elem_lifetime->policy->copy);
	assert(key_lifetime->policy->hash && key_lifetime->policy->equals);

	void* buffer = nullptr;
	void* keys_buffer = nullptr;
	void* meta_buffer = nullptr;
	if(capacity != 0) {
		buffer = allocator_alloc(
			allocator,
			elem_size * capacity,
			elem_alignment
		);
		if(!buffer) { goto empty_return; }

		keys_buffer = allocator_alloc(
			allocator,
			key_size * capacity,
			key_alignment
		);
		if(!keys_buffer) { goto free_buffer; }

		meta_buffer = allocator_alloc(
			allocator,
			capacity * sizeof(HashMapSlotState),
			alignof(HashMapSlotState)
		);
		if(!meta_buffer) { goto free_keys_buffer; }

		// TODO: Figure out whether I can use memset once
		for(usize i = 0; i < capacity; i++) {
			*((HashMapSlotState*)meta_buffer + i) = HASHMAP_SLOT_EMPTY;
		}
		// (void)memset(meta_buffer, HASHMAP_SLOT_EMPTY, capacity);
	}
	return (HashMap){
		.buffer = buffer,
		.keys_buffer = keys_buffer,
		.meta_buffer = meta_buffer,
		.elem_count = 0,
		.descriptor = (HashMapDescriptor){
			.capacity = capacity,
			.key_size = key_size,
			.elem_size = elem_size,
			.key_alignment = key_alignment,
			.elem_alignment = elem_alignment,
			.hash_strategy = hash_strategy,
			.allocator = allocator,
			.key_lifetime = key_lifetime,
			.elem_lifetime = elem_lifetime,
		},
	};
	free_keys_buffer: allocator_free(allocator, keys_buffer);
	free_buffer: allocator_free(allocator, buffer);
	empty_return: return (HashMap){0};
}

void hashmap_destroy(HashMap* hashmap) {
    if(hashmap->buffer) {
        hashmap_clear(hashmap);
        allocator_free(hashmap->descriptor.allocator, hashmap->buffer);
        allocator_free(hashmap->descriptor.allocator, hashmap->keys_buffer);
        allocator_free(hashmap->descriptor.allocator, hashmap->meta_buffer);
    }
    hashmap_reset_state(hashmap);
}


internal void hashmap_relocate_buffers(
	const HashMap* hashmap,
	void* dest_buffer,
	void* dest_keys_buffer,
	void* dest_meta_buffer,
	const void* src_buffer,
	const void* src_keys_buffer,
	const void* src_meta_buffer,
	usize capacity
) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime && key_lifetime->policy && key_lifetime->policy->hash);
	assert(!elem_lifetime || elem_lifetime->policy->move || elem_lifetime->policy->copy);

	if(key_lifetime->policy->copy) {
		for(usize i = 0; i < capacity; i++) {
			key_lifetime->policy->copy(
				key_lifetime->ctx,
				(u8*)dest_keys_buffer + i * hashmap->descriptor.key_size,
				(u8*)src_keys_buffer + i * hashmap->descriptor.key_size
			);
		}
	} else {
		(void)memmove(
			dest_keys_buffer,
			src_keys_buffer,
			hashmap->descriptor.key_size * capacity
		);
	}

	if(!elem_lifetime) {
		(void)memmove(
			dest_buffer,
			src_buffer,
			hashmap->descriptor.elem_size * capacity
		);
	} else if(elem_lifetime->policy->move) {
		for(usize i = 0; i < capacity; i++) {
			elem_lifetime->policy->move(
				elem_lifetime->ctx,
				(u8*)dest_buffer + i * hashmap->descriptor.elem_size,
				(u8*)src_buffer + i * hashmap->descriptor.elem_size
			);
		}
	} else {
		for(usize i = 0; i < capacity; i++) {
			elem_lifetime->policy->copy(
				elem_lifetime->ctx,
				(u8*)dest_buffer + i * hashmap->descriptor.elem_size,
				(u8*)src_buffer + i * hashmap->descriptor.elem_size
			);
		}
	}

	(void)memmove(
		dest_meta_buffer,
		src_meta_buffer,
		sizeof(HashMapSlotState) * capacity
	);
}

internal void hashmap_rehash(
	void* dest_buffer,
	void* dest_keys_buffer,
	void* dest_meta_buffer,
	usize new_capacity,
	const HashMap* src_hashmap
) {
	if(src_hashmap->descriptor.hash_strategy != HASHMAP_HASH_STRATEGRY_RECOMPUTE) {
		return;
	}

	const ElementLifetime* key_lifetime = src_hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = src_hashmap->descriptor.elem_lifetime;

	(void)memset(dest_meta_buffer, HASHMAP_SLOT_EMPTY, new_capacity);

	// Copy every element
	for(usize i = 0; i < src_hashmap->descriptor.capacity; i++) {

		const void* src_key = (u8*)src_hashmap->keys_buffer + i * src_hashmap->descriptor.key_size;
		const void* src_elem = (u8*)src_hashmap->buffer + i * src_hashmap->descriptor.elem_size;
		HashMapSlotState src_state = *((HashMapSlotState*)src_hashmap->meta_buffer + i);
		if(src_state != HASHMAP_SLOT_OCCUPIED) {
			continue;
		}

		hash_t index = hashmap_freestanding_hash(src_hashmap, src_key, new_capacity);
		hash_t tombstone = INVALID_INDEX;

		bool exitFor = false;
		for(usize probes = 0; probes < new_capacity; probes++) {
			if(exitFor) { break; }
			void* key = (u8*)dest_keys_buffer + index * src_hashmap->descriptor.key_size;
			void* elem = (u8*)dest_buffer + index * src_hashmap->descriptor.elem_size;
			HashMapSlotState* state = (HashMapSlotState*)dest_meta_buffer + index;

			switch(*state) {
				case HASHMAP_SLOT_EMPTY:
					if(tombstone != INVALID_INDEX) {
						index = tombstone;
					}
					hashmap_insert_at_location(
						src_hashmap,
						elem,
						src_elem,
						key,
						src_key,
						true
					);
					if(key_lifetime && key_lifetime->policy->dtor) {
						key_lifetime->policy->dtor(
							key_lifetime->ctx,
							(void*)src_key
						);
					}
					if(elem_lifetime && elem_lifetime->policy->dtor) {
						elem_lifetime->policy->dtor(
							elem_lifetime->ctx,
							(void*)src_elem
						);
					}
					*state = HASHMAP_SLOT_OCCUPIED;
					exitFor = true;
					break;

				case HASHMAP_SLOT_OCCUPIED:
					index = (index + 1) % new_capacity;
					break;

				case HASHMAP_SLOT_TOMBSTONE:
					UNREACHABLE("Invalid hashmap slot state HASHMAP_SLOT_TOMBSTONE in `rehash`");

				default:
					UNREACHABLE("Invalid hashmap slot state %u at index %zu", *state, index);
			}
		}
	}

	// Construct empty slots
	for(usize i = 0; i < new_capacity; i++) {
		HashMapSlotState* state = (HashMapSlotState*)dest_meta_buffer + i;
		if(*state == HASHMAP_SLOT_OCCUPIED) {
			continue;
		}
		void* elem = (u8*)dest_buffer + i * src_hashmap->descriptor.elem_size;
		if(elem_lifetime) {
			elem_lifetime->policy->ctor(
				elem_lifetime->ctx,
				elem
			);
		}
		*state = HASHMAP_SLOT_EMPTY;
	}
}

// NOTE: Copies every element from src to dest
// This can fall into one of the following cases
// - POD type				-> copied using `memset`
// - Non-copyable type		-> function aborts and returns `false`
// - Policy-copyable type	-> copied using policy's copy function
bool hashmap_copy(
    HashMap* dest,
    const HashMap* src
) {
	return hashmap_copy_walloc(dest, src, src->descriptor.allocator);
}
bool hashmap_copy_walloc(
    HashMap* dest,
    const HashMap* src,
	const Allocator* allocator
) {
	const ElementLifetime* elem_lifetime = src->descriptor.elem_lifetime;

	assert(!elem_lifetime || elem_lifetime->policy);

	if(elem_lifetime && !elem_lifetime->policy->copy) {
		return false;
	}

	void* buffer = allocator_alloc(
		allocator,
		src->descriptor.elem_size * src->descriptor.capacity,
		src->descriptor.elem_alignment
	);
	if(!buffer) { goto empty_return; }
	void* keys_buffer = allocator_alloc(
		allocator,
		src->descriptor.key_size * src->descriptor.capacity,
		src->descriptor.key_alignment
	);
	if(!keys_buffer) { goto free_buffer; }
	void* meta_buffer = allocator_alloc(
		allocator,
		src->descriptor.capacity* sizeof(HashMapSlotState),
		alignof(HashMapSlotState)
	);
	if(!meta_buffer) { goto free_keys_buffer; }

	hashmap_relocate_buffers(
		(HashMap*)src, // TODO: Probably somethings about this (figure out const pointers)
		buffer,
		keys_buffer,
		meta_buffer,
		src->buffer,
		src->keys_buffer,
		src->meta_buffer,
		src->descriptor.capacity
	);

	dest->buffer = buffer;
	dest->meta_buffer = meta_buffer;
	dest->keys_buffer = keys_buffer;

	dest->elem_count = src->elem_count;
	dest->descriptor = src->descriptor;
	dest->descriptor.allocator = allocator;

	return true;

	free_keys_buffer: allocator_free(allocator, keys_buffer);
	free_buffer: allocator_free(allocator, buffer);
	empty_return: return false;
}
// NOTE: Moves the whole array, not the elements (ownership transfer)
// WARN: Empties `src`
void hashmap_move(
	HashMap* dest,
	HashMap* src
) {
	hashmap_destroy(dest);
	*dest = *src;
	hashmap_reset_state(src);
}


// --= Size =--

bool hashmap_grow(HashMap* hashmap, usize new_capacity) {

	if(new_capacity < hashmap->descriptor.capacity) {
		return false;
	}
	if(new_capacity == hashmap->descriptor.capacity) {
		return true;
	}

	if(new_capacity == INVALID_INDEX) {
		LOG("Capacity: %zu, new capacity: %zu", hashmap->descriptor.capacity, hashmap->descriptor.capacity * HASHMAP_GROW_FACTOR);
		new_capacity = hashmap->descriptor.capacity == 0
			? 1
			: hashmap->descriptor.capacity * HASHMAP_GROW_FACTOR;
	}

	const Allocator* alloc = hashmap->descriptor.allocator;

	void* buffer = allocator_alloc(
		alloc,
		hashmap->descriptor.elem_size * new_capacity,
		hashmap->descriptor.elem_alignment
	);
	if(!buffer) { goto empty_return; }
	void* keys_buffer = allocator_alloc(
		alloc,
		hashmap->descriptor.key_size * new_capacity,
		hashmap->descriptor.key_alignment
	);
	if(!keys_buffer) { goto free_buffer; }
	void* meta_buffer = allocator_alloc(
		alloc,
		new_capacity * sizeof(HashMapSlotState),
		alignof(HashMapSlotState)
	);
	if(!meta_buffer) { goto free_keys_buffer; }

	hashmap_rehash(
		buffer,
		keys_buffer,
		meta_buffer,
		new_capacity,
		hashmap
	);

	hashmap->buffer = buffer;
	hashmap->keys_buffer = keys_buffer;
	hashmap->meta_buffer = meta_buffer;
	hashmap->descriptor.capacity = new_capacity;

	return true;

	free_keys_buffer: allocator_free(hashmap->descriptor.allocator, keys_buffer);
	free_buffer: allocator_free(hashmap->descriptor.allocator, buffer);
	empty_return: return false;
}


// --= Element Access =--

void* hashmap_at(const HashMap* hashmap, const void* key) {
	if(!hashmap_has(hashmap, key)) {
		return nullptr;
	}

	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;

	hash_t index = hashmap_hash(hashmap, key);

	for(usize probes = 0; probes < hashmap->descriptor.capacity; probes++) {
		const HashMapSlotState state = hashmap_get_slot_state(hashmap, index);
        switch(state) {
            case HASHMAP_SLOT_EMPTY:
				return nullptr;

            case HASHMAP_SLOT_OCCUPIED:
				if(key_lifetime->policy->equals(
					key_lifetime->ctx,
					key,
					hashmap_get_key(hashmap, index)
				)) {
					return (void*)((u8*)hashmap->buffer + index * hashmap->descriptor.elem_size);
                }
				[[fallthrough]];

            case HASHMAP_SLOT_TOMBSTONE:
                index = (index + 1) % hashmap->descriptor.capacity;
                break;

            default:
                UNREACHABLE("Invalid hashmap slot state %u at index %zu", state, index);
        }
	}

	UNREACHABLE("Hashmap full at %p", hashmap);
}
const void* hashmap_at_const(const HashMap* hashmap, const void* key) {
	return (const void*)hashmap_at(hashmap, key);
}

bool hashmap_has(const HashMap *hashmap, const void *key) {

	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime && key_lifetime->policy && key_lifetime->policy->hash);
	assert(!elem_lifetime || elem_lifetime->policy);

	hash_t index = hashmap_hash(hashmap, key);

	for(usize probes = 0; probes < hashmap->descriptor.capacity; probes++) {
		const HashMapSlotState state = hashmap_get_slot_state(hashmap, index);
        switch(state) {
            case HASHMAP_SLOT_EMPTY:
				return false;

            case HASHMAP_SLOT_OCCUPIED:
				if(key_lifetime->policy->equals(
					key_lifetime->ctx,
					key,
					hashmap_get_key(hashmap, index)
				)) {
					return true;
                }
				[[fallthrough]];

            case HASHMAP_SLOT_TOMBSTONE:
                index = (index + 1) % hashmap->descriptor.capacity;
                break;

            default:
                UNREACHABLE("Invalid hashmap slot state %u at index %zu", state, index);
        }
	}

	UNREACHABLE("Hashmap full at %p", hashmap);
}


// --= Modifiers =--

internal bool hashmap_insert_any(HashMap* hashmap, void* key, void* elem, bool force_move) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime && key_lifetime->policy);
	assert(key_lifetime->policy->hash && key_lifetime->policy->equals);
	assert(!elem_lifetime || elem_lifetime->policy);

	if(hashmap->elem_count >= hashmap->descriptor.capacity) {
		if(!hashmap_grow(hashmap, INVALID_INDEX)) {
			return false;
		}
	}

	hash_t index = hashmap_hash(hashmap, key);

	hash_t tombstone = INVALID_INDEX;

	for(usize probes = 0; probes < hashmap->descriptor.capacity; probes++) {
		const HashMapSlotState state = hashmap_get_slot_state(hashmap, index);
        switch(state) {
            case HASHMAP_SLOT_EMPTY:
				if(tombstone != INVALID_INDEX) {
					index = tombstone;
				}
                hashmap_insert_at_slot(hashmap, index, key, elem, force_move);

                return true;

            case HASHMAP_SLOT_TOMBSTONE:
				tombstone = index;

                index = (index + 1) % hashmap->descriptor.capacity;
                break;

            case HASHMAP_SLOT_OCCUPIED:
                if(key_lifetime->policy->equals(
					key_lifetime->ctx,
					key,
					hashmap_get_key(hashmap, index)
				)) {
					hashmap_remove_at_slot(hashmap, index);
                    hashmap_insert_at_slot(hashmap, index, key, elem, force_move);

                    return true;
                }

                index = (index + 1) % hashmap->descriptor.capacity;
                break;

            default:
                UNREACHABLE("Invalid hashmap slot state %u at index %zu", state, index);
        }
	}

	UNREACHABLE("Hashmap full at %p, %zu", hashmap, index);
}

internal void hashmap_insert_at_location(
	const HashMap* hashmap,
	void* dest_elem,
	const void* src_elem,
	void* dest_key,
	const void* src_key,
	bool force_move
) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	if(force_move) {
		if(key_lifetime->policy->move) {
			key_lifetime->policy->move(
				key_lifetime->ctx,
				dest_key,
				(void*)src_key
			);
		} else if(key_lifetime->policy->copy) {
			key_lifetime->policy->copy(
				key_lifetime->ctx,
				dest_key,
				src_key
			);
		} else {
			(void)memmove(
				dest_key,
				src_key,
				hashmap->descriptor.key_size
			);
		}
	} else {
		if(key_lifetime->policy->copy) {
			key_lifetime->policy->copy(
				key_lifetime->ctx,
				dest_key,
				src_key
			);
		} else {
			(void)memmove(
				dest_key,
				src_key,
				hashmap->descriptor.key_size
			);
		}
	}

	if(force_move) {
		if(elem_lifetime && elem_lifetime->policy->move) {
			elem_lifetime->policy->move(
				elem_lifetime->ctx,
				dest_elem,
				(void*)src_elem
			);
		} else if(elem_lifetime && elem_lifetime->policy->copy) {
			elem_lifetime->policy->copy(
				elem_lifetime->ctx,
				dest_elem,
				src_elem
			);
		} else {
			(void)memmove(
				dest_elem,
				src_elem,
				hashmap->descriptor.elem_size
			);
		}
	} else {
		if(elem_lifetime && elem_lifetime->policy->copy) {
			elem_lifetime->policy->copy(
				elem_lifetime->ctx,
				dest_elem,
				src_elem
			);
		} else {
			(void)memmove(
				dest_elem,
				src_elem,
				hashmap->descriptor.elem_size
			);
		}
	}
}

internal void hashmap_insert_at_slot(
	HashMap* hashmap,
	hash_t index,
	const void* key,
	const void* elem,
	bool force_move
) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime);
	assert(!elem_lifetime || elem_lifetime->policy->move || elem_lifetime->policy->copy);

	hashmap_insert_at_location(
		hashmap,
		hashmap_get_elem(hashmap, index),
		elem,
		hashmap_get_key(hashmap, index),
		key,
		force_move
	);

	hashmap_set_slot_state(hashmap, index, HASHMAP_SLOT_OCCUPIED);

	hashmap->elem_count++;
}

internal void hashmap_remove_at_slot(HashMap* hashmap, hash_t index) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime);
	assert(!elem_lifetime || elem_lifetime->policy);

	if(!key_lifetime->policy->dtor) {
		(void)memset(
			hashmap_get_key(hashmap, index),
			0,
			hashmap->descriptor.key_size
		);
	} else {
		key_lifetime->policy->dtor(
			key_lifetime->ctx,
			hashmap_get_key(hashmap, index)
		);
	}

	if(!elem_lifetime || !elem_lifetime->policy->dtor) {
		(void)memset(
			hashmap_get_elem(hashmap, index),
			0,
			hashmap->descriptor.elem_size
		);
	} else {
		elem_lifetime->policy->dtor(
			elem_lifetime->ctx,
			hashmap_get_elem(hashmap, index)
		);
	}

	hashmap_set_slot_state(hashmap, index, HASHMAP_SLOT_TOMBSTONE);
	hashmap->elem_count--;
}


// WARN: In the case of a movable policy datatype, elem is cast to `void*` and invalidated
bool hashmap_insert(HashMap* hashmap, const void* key, const void* elem) {
	return hashmap_insert_any(hashmap, (void*)key, (void*)elem, false);
}

bool hashmap_insert_move(HashMap* hashmap, void* key, void* elem) {
	return hashmap_insert_any(hashmap, key, elem, true);
}
bool hashmap_remove(HashMap* hashmap, const void* key) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime && key_lifetime->policy);
	assert(key_lifetime->policy->hash && key_lifetime->policy->equals);
	assert(!elem_lifetime || elem_lifetime->policy);

	if(hashmap->elem_count == 0) {
		return false;
	}

	hash_t index = hashmap_hash(hashmap, key);

	for(usize probes = 0; probes < hashmap->descriptor.capacity; probes++) {
		const HashMapSlotState state = hashmap_get_slot_state(hashmap, index);
        switch(state) {
            case HASHMAP_SLOT_EMPTY:
				return false;

            case HASHMAP_SLOT_OCCUPIED:
                if(key_lifetime->policy->equals(
					key_lifetime->ctx,
					key,
					hashmap_get_key(hashmap, index)
				)) {
                    hashmap_remove_at_slot(hashmap, index);
                    return true;
                }
				[[fallthrough]];

            case HASHMAP_SLOT_TOMBSTONE:
                index = (index + 1) % hashmap->descriptor.capacity;
                break;

            default:
                UNREACHABLE("Invalid hashmap slot state %u at index %zu", state, index);
        }
	}

	UNREACHABLE("Hashmap full at %p", hashmap);
}

void hashmap_clear(HashMap* hashmap) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime && key_lifetime->policy);
	assert(key_lifetime->policy->hash && key_lifetime->policy->equals);
	assert(!elem_lifetime || elem_lifetime->policy);

	if(hashmap->elem_count == 0) {
		return;
	}

	for(usize i = 0; i < hashmap->descriptor.capacity; i++) {
		const HashMapSlotState state = hashmap_get_slot_state(hashmap, i);
		if(state == HASHMAP_SLOT_OCCUPIED) {
			hashmap_remove_at_slot(hashmap, i);
		}
		if(state != HASHMAP_SLOT_EMPTY) {
			hashmap_set_slot_state(hashmap, i, HASHMAP_SLOT_EMPTY);
		}
	}
}

internal void hashmap_reset_state(HashMap* hashmap) {
    hashmap->buffer = nullptr;
    hashmap->keys_buffer = nullptr;
	hashmap->meta_buffer = nullptr;
	hashmap->elem_count = 0;
    hashmap->descriptor.capacity = 0;
    hashmap->descriptor.key_size = 0;
    hashmap->descriptor.elem_size = 0;
    hashmap->descriptor.key_alignment = 0;
    hashmap->descriptor.elem_alignment = 0;
    hashmap->descriptor.allocator = nullptr;
	hashmap->descriptor.hash_strategy = HASHMAP_HASH_STRATEGRY_RECOMPUTE;
    hashmap->descriptor.key_lifetime = nullptr;
    hashmap->descriptor.elem_lifetime = nullptr;
}
internal void hashmap_reset_destructive(HashMap* hashmap) {
    hashmap_clear(hashmap);
    hashmap_reset_state(hashmap);
}
