# Foundation

## Containers

### DynArray ✅

## Core

### Tests

## Macros

## Memory

### Reserve / Release API ✅

- `malloc` (uses `aligned_alloc` when alignment required)
- `cmalloc` (`malloc` with custom alignment using header)
- `mmap` (UNIX-only)

### Allocator API ✅

```
Allocator:
 handler: <ArenaCtx>
 vt: { alloc, free, realloc, reset }
 source: <MemorySource>
```

### Arena Allocator ✅

### Stack Allocator ✅

A stack allocator is a **Last-In, First-Out (LIFO)** allocator.

Allocations are made sequentially from a contiguous buffer, and only the **most recently allocated block** may be freed or reallocated.

Unlike an arena allocator, a stack allocator supports individual `free()` and `realloc()`, but only on the current top allocation.

---

### Memory Layout

Each allocation consists of four regions:

```text
+-----------+----------------+---------+-----------+
|  Padding  |  StackHeader   | Padding | User Data |
+-----------+----------------+---------+-----------+
```

The layout satisfies the following constraints:

1. `StackHeader` is aligned to `alignof(StackHeader)`.
2. User data is aligned to the requested alignment.
3. Given only the user pointer, the allocator can recover the corresponding `StackHeader`.

Because the header and user data may have different alignment requirements, they are **not necessarily adjacent in memory**. Padding is inserted before the header so that both alignment constraints are satisfied.

---

### Complexity

| Operation | Complexity |
|-----------|-----------:|
| Allocate | O(1) |
| Free (top only) | O(1) |
| Reallocate (top only) | O(1) |
| Reset | O(1) |

---

### Characteristics

> Advantages

- Extremely fast allocations.
- Constant-time free.
- Constant-time reallocation of the top allocation.
- No fragmentation.
- Very small metadata.

> Limitations

- Allocations must be freed in reverse order.
- Cannot free arbitrary allocations.
- Cannot reallocate non-top allocations.
- Capacity is fixed unless backed by a growing memory source.

### Pool Allocator ❌

### Slab Allocator ❌

### Free-List Allocator ❌

"malloc replacement"

- Blocks with headers
- Linked free list(s)
- Splitting/merging
- Binning by size classes

### Buddy Allocator ❌

## Strings

# Math

## Geometry

## Linear

## Random

# Network

# Runtime

## File System

## OS

## Threading

## Time
