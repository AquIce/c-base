#include <base/foundation/containers/hashmap.h>

#include <base/foundation/macros.h>
#include <base/foundation/containers/container.h>
#include <base/foundation/memory/allocator.h>

#include <assert.h>

// --= Local Header =--

internal const usize INVALID_INDEX = (usize)(-1);

typedef enum {
    HASHMAP_SLOT_EMPTY     = 0,
    HASHMAP_SLOT_OCCUPIED  = 1,
    HASHMAP_SLOT_TOMBSTONE = 2,
} HashMapSlotState;

internal_fn usize hashmmap_entry_size(const HashMap* hashmap) {
	return hashmap->descriptor.key_size + hashmap->descriptor.elem_size;
}

internal_fn i32 hashmap_get_slot_state(const HashMap* hashmap, usize index) {
	return *(i32*)((u8*)hashmap->meta_buffer + index);
}
internal_fn void* hashmap_get_key(const HashMap* hashmap, usize index) {
	return (
		(u8*)hashmap->buffer + index * hashmmap_entry_size(hashmap)
	);
}
internal_fn void* hashmap_get_elem(const HashMap* hashmap, usize index) {
	return (
		(u8*)hashmap->buffer + index * hashmmap_entry_size(hashmap) + hashmap->descriptor.key_size
	);
}

internal void hashmap_insert_at_slot(HashMap* hashmap, usize index, const void* key, const void* elem);

internal void hashmap_reset_state(HashMap* hashmap);
internal void hashmap_reset_destructive(HashMap* hashmap);

// --= Element Lifetime =--

internal void hashmap_policy_ctor(void* ctx, void* elem) {
	if(!ctx || !elem) { return; }
	HashMapDescriptor* descriptor = (HashMapDescriptor*)ctx;
	*(HashMap*)elem = hashmap_create_complex(
		descriptor->allocator,
		descriptor->capacity,
		descriptor->key_size,
		descriptor->elem_size,
		descriptor->alignment,
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
internal usize hashmap_policy_hash(void* ctx, const void* object) {
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
	usize alignment,
	const ElementLifetime* key_lifetime
) {
	return hashmap_create_complex(
		allocator,
		capacity,
		key_size,
		elem_size,
		alignment,
		key_lifetime,
		POD_LIFETIME
	);
}
HashMap hashmap_create_complex(
	const Allocator* allocator,
	usize capacity,
	usize key_size,
	usize elem_size,
	usize alignment,
	const ElementLifetime* key_lifetime,
	const ElementLifetime* elem_lifetime
) {
	assert(key_lifetime && key_lifetime->policy);
	assert(!elem_lifetime || elem_lifetime->policy);

	assert(key_lifetime->policy->hash && key_lifetime->policy->equals);

	void* buffer = nullptr;
	void* meta_buffer = nullptr;
	if(capacity != 0) {
		buffer = allocator_alloc(allocator, (key_size + elem_size) * capacity, alignment);
		if(!buffer) {
			return (HashMap){0};
		}
		meta_buffer = allocator_alloc(allocator, capacity, alignof(HashMapSlotState));
		if(!meta_buffer) {
			allocator_free(allocator, buffer);
			return (HashMap){0};
		}
	}
	return (HashMap){
		.buffer = buffer,
		.meta_buffer = meta_buffer,
		.elem_count = 0,
		.descriptor = (HashMapDescriptor){
			.capacity = capacity,
			.key_size = key_size,
			.elem_size = elem_size,
			.alignment = alignment,
			.allocator = allocator,
			.key_lifetime = key_lifetime,
			.elem_lifetime = elem_lifetime,
		},
	};
}

void hashmap_destroy(HashMap* hashmap) {
    if(hashmap->buffer) {
        hashmap_clear(hashmap);
        allocator_free(hashmap->descriptor.allocator, hashmap->buffer);
    }
    hashmap_reset_state(hashmap);
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
	TODO_IMPL();
}
// NOTE: Moves the whole array, not the elements (ownership transfer)
// WARN: Empties `src`
void hashmap_move(
	HashMap* dest,
	HashMap* src
) {
	TODO_IMPL();
}


// --= Size =--

bool hashmap_reserve(HashMap* hashmap, usize capacity) {
	TODO_IMPL();
}

// --= Modifiers =--

internal void hashmap_insert_at_slot(HashMap* hashmap, usize index, const void* key, const void* elem) {
	TODO_IMPL();
}

// WARN: In the case of a movable policy datatype, elem is cast to `void*` and invalidated
bool hashmap_insert(HashMap* hashmap, const void* key, const void* elem) {
	const ElementLifetime* key_lifetime = hashmap->descriptor.key_lifetime;
	const ElementLifetime* elem_lifetime = hashmap->descriptor.elem_lifetime;

	assert(key_lifetime && key_lifetime->policy);
	assert(key_lifetime->policy->hash && key_lifetime->policy->equals);
	assert(!elem_lifetime || elem_lifetime->policy);

	HashFunc hashFunc = key_lifetime->policy->hash;
	EqualsFunc equalsFunc = key_lifetime->policy->equals;

	if(hashmap->elem_count + 1 >= hashmap->descriptor.capacity) {
		TODO("Grow");
	}

	usize index = hashFunc(key_lifetime->ctx, key) % hashmap->descriptor.capacity;

	usize tombstone = INVALID_INDEX;

	for(usize probes = 0; probes < hashmap->descriptor.capacity; probes++) {
        switch(hashmap_get_slot_state(hashmap, index)) {
            case HASHMAP_SLOT_EMPTY:
				if(tombstone != INVALID_INDEX) {
					index = tombstone;
				}
                hashmap_insert_at_slot(hashmap, index, key, elem);
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
                    hashmap_insert_at_slot(hashmap, index, key, elem);
                    return true;
                }

                index = (index + 1) % hashmap->descriptor.capacity;
                break;

            default:
                UNREACHABLE("Invalid hashmap slot state");
        }
	}

	UNREACHABLE("Hashmap full");
}

bool hashmap_remove(HashMap* hashmap, const void* key) {
	TODO_IMPL();
}

void hashmap_clear(HashMap* hashmap) {
	TODO_IMPL();
}

internal void hashmap_reset_state(HashMap* hashmap) {
    hashmap->buffer = nullptr;
	hashmap->meta_buffer = nullptr;
	hashmap->elem_count = 0;
    hashmap->descriptor.capacity = 0;
    hashmap->descriptor.key_size = 0;
    hashmap->descriptor.elem_size = 0;
    hashmap->descriptor.alignment = 0;
    hashmap->descriptor.allocator = nullptr;
    hashmap->descriptor.key_lifetime = nullptr;
    hashmap->descriptor.elem_lifetime = nullptr;
}
internal void hashmap_reset_destructive(HashMap* hashmap) {
    hashmap_clear(hashmap);
    hashmap_reset_state(hashmap);
}
