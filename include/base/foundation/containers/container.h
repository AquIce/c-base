#ifndef __BASE_FOUNDATION_CONTAINERS__
#define __BASE_FOUNDATION_CONTAINERS__

#include <base/foundation/macros.h>

#define POD_LIFETIME nullptr

typedef void  (*CtorFunc)(void* ctx, void* elem);
typedef void  (*DtorFunc)(void* ctx, void* elem);
typedef void  (*CopyFunc)(void* ctx, void* dest, const void* src);
typedef void  (*MoveFunc)(void* ctx, void* dest, void* src);
typedef bool  (*EqualsFunc)(void* ctx, const void* elem, const void* other);
typedef i32   (*CompareFunc)(void* ctx, const void* elem, const void* other);
typedef usize (*HashFunc)(void* ctx, const void* object);

// WARN: Invariant: `assert(!policy || (policy->ctor && policy->dtor));`
// WARN: Invariant: `assert(!policy || policy->equals);`
typedef struct {
	CtorFunc ctor;
	DtorFunc dtor;

	// NOTE: If copy is not provided, the element described by this policy is considered NOT COPYABLE
	CopyFunc copy;
	// NOTE: If move is not provided, the element described by this policy is considered NOT MOVABLE
	// NOTE: Move leaves the source element destroyed (no need to call `ctor`)
	MoveFunc move;

	// WARN: Important: equals and compare need to provide the same results (`equals -> true` => `compare -> 0`)
	EqualsFunc equals;
	CompareFunc compare;

	HashFunc hash;
} ElementPolicy;

// WARN: Invariant: `assert(!lifetime || lifetime->policy);`
typedef struct {
	const ElementPolicy* policy;
	void* ctx;
} ElementLifetime;

#endif // __BASE_FOUNDATION_CONTAINERS__
