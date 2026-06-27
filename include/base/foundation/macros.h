#ifndef __BASE_FOUNDATION_MACROS__
#define __BASE_FOUNDATION_MACROS__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ptrdiff_t isize;

typedef uintptr_t uptr;
typedef intptr_t iptr;

#define internal static
#define persistent static
#define internal_fn static inline

#define nullptr (void*)NULL
#define PTR_SIZE sizeof(void*)
#define PTR_ALIGNMENT alignof(max_align_t)

#define UNUSED(x) (void)(x)

#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)
#define GB(x) ((x) * 1024ULL * 1024ULL * 1024ULL)

#endif // __BASE_FOUNDATION_MACROS__
