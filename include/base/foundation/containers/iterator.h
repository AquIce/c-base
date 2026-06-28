#ifndef __BASE_FOUNDATION_CONTAINERS_ITERATOR__
#define __BASE_FOUNDATION_CONTAINERS_ITERATOR__

typedef struct {
    void* (*get)(void* ctx);
    void (*next)(void* ctx);
    bool (*done)(void* ctx);
} IteratorVTable;

typedef struct {
	void* ctx;
	const IteratorVTable* vt;
} Iterator;

#endif // __BASE_FOUNDATION_CONTAINERS_ITERATOR__
