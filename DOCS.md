# Foundation

## Containers

### DynArray

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

### Stack Allocator ❌

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
