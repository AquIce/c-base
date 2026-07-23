#ifndef __BASE_FOUNDATION_MACROS__
#define __BASE_FOUNDATION_MACROS__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
#define MAX_ALIGNMENT alignof(max_align_t)

#define LOG(fmt, ...) \
    fprintf(stderr, "[%s:%d] %s: " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define TODO(fmt, ...) \
    do { \
        fprintf(stderr, "TODO: " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__); \
        abort(); \
    } while(0)

#define TODO_IMPL() \
    TODO("%s has not been implemented yet", __func__)

#define comptime_assert _Static_assert

#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)
#define GB(x) ((x) * 1024ULL * 1024ULL * 1024ULL)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifndef UNREACHABLE
#  if defined(__GNUC__) || defined(__clang__)
#    define UNREACHABLE(reason) \
        do { \
            fprintf(stderr, "UNREACHABLE: %s (%s:%d)\n", (reason), __FILE__, __LINE__); \
            __builtin_unreachable(); \
        } while(0)
#  else
#    define UNREACHABLE(reason) \
        do { \
            fprintf(stderr, "UNREACHABLE: %s (%s:%d)\n", (reason), __FILE__, __LINE__); \
            abort(); \
        } while(0)
#  endif
#endif

#endif // __BASE_FOUNDATION_MACROS__
