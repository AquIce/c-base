#ifndef __BASE_FOUNDATION_CONTAINERS__
#define __BASE_FOUNDATION_CONTAINERS__

#include <base/foundation/macros.h>

#define POD_LIFETIME nullptr

// WARN: Invariant: `assert(!policy || (policy->ctor && policy->dtor));`
typedef struct {
    void (*ctor)(void* ctx, void* elem);
    void (*dtor)(void* ctx, void* elem);
	// NOTE: If copy is not provided, the element described by this policy is considered NOT COPYABLE
    void (*copy)(void* ctx, void *dest, const void *src);
	// NOTE: If move is not provided, the element described by this policy is considered NOT MOVABLE
	// NOTE: Move leaves the source element destroyed (no need to call `ctor`)
	void (*move)(void* ctx, void *dest, void *src);
    // void (*compare)(void* ctx, const void *elem, const void *other);
} ElementPolicy;

// WARN: Invariant: `assert(!lifetime || lifetime->policy);`
typedef struct {
	const ElementPolicy* policy;
	void* ctx;
} ElementLifetime;

#endif // __BASE_FOUNDATION_CONTAINERS__
