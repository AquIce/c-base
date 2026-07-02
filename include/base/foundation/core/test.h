#ifndef __BASE_FOUNDATION_CORE_TEST__
#define __BASE_FOUNDATION_CORE_TEST__

#include <base/foundation/macros.h>

#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_FAIL_FATAL

typedef void (*TestFn)(void);
typedef void (*SetupFn)(void);
typedef void (*TeardownFn)(void);

typedef struct TestNode {
    const char* name;

    TestFn fn; // nullptr if group
    struct TestNode* children;
    int child_count;

    SetupFn setup;
    TeardownFn teardown;
} TestNode;

#define TEST(name) void name(void)

#define TEST_PROGRAM() \
int main(void) { \
    extern TestNode* __start_test_roots[]; \
    extern TestNode* __stop_test_roots[]; \
    for(usize i = 0; i < (__stop_test_roots - __start_test_roots); i++) \
        run_node(__start_test_roots[i]); \
    return tests_failed != 0; \
}

#if defined(__GNUC__) || defined(__clang__)
#define TEST_ROOT(id, name_str, setup_fn, teardown_fn, ...) \
    static TestNode id##_root = { \
        .name = name_str, \
        .fn = nullptr, \
        .setup = setup_fn, \
        .teardown = teardown_fn, \
        .children = (TestNode[]){ __VA_ARGS__ }, \
        .child_count = sizeof((TestNode[]){ __VA_ARGS__ }) / sizeof(TestNode) \
    }; \
    __attribute__((used, section("test_roots"))) \
    static TestNode* id##_root_ptr = &id##_root;
#else
#error "Linker-section TEST_ROOT requires GCC/Clang"
#endif

#define TEST_GROUP(name, ...) \
    TEST_GROUP_EX(name, nullptr, nullptr, __VA_ARGS__)

#define TEST_GROUP_EX(name_str, setup_fn, teardown_fn, ...) \
    (TestNode){ \
        .name = name_str, \
        .fn = nullptr, \
        .setup = setup_fn, \
        .teardown = teardown_fn, \
        .children = (TestNode[]){ __VA_ARGS__ }, \
        .child_count = sizeof((TestNode[]){ __VA_ARGS__ }) / sizeof(TestNode) \
    }

#define TEST_NODE(fn_name) \
    (TestNode){ \
		.name = #fn_name, \
		.fn = fn_name, \
		.children = nullptr, \
		.child_count = 0 \
	}

internal int tests_run = 0;
internal int tests_failed = 0;

#ifdef TEST_FAIL_FATAL
#define TEST_FAIL_RESULT assert(false)
#else
#define TEST_FAIL_RESULT return
#endif

#define TEST_FAIL(fn, file, line, fmt, ...) \
	do { \
		fprintf(stderr, "[FAIL] %s:%d (%s): " fmt "\n", file, line, fn, ##__VA_ARGS__); \
		tests_failed++; \
		TEST_FAIL_RESULT; \
	} while(0)

#define ASSERT_TRUE(expr) \
	do { \
		if(!(expr)) { \
			TEST_FAIL(__func__, __FILE__, __LINE__, "Expected TRUE but was FALSE: %s", #expr); \
		} \
	} while(0)

#define DEF_ASSERT_EQ_INT(T, fmt) \
internal_fn void assert_eq_##T(const char* fn, const char* file, int line, const char* ea, const char* eb, T a, T b) { \
    if(a != b) { \
        TEST_FAIL(fn, file, line, "expected %s=" fmt ", got %s=" fmt, ea, a, eb, b); \
    } \
}

DEF_ASSERT_EQ_INT(i8,  "%d")
DEF_ASSERT_EQ_INT(i16, "%d")
DEF_ASSERT_EQ_INT(i32, "%d")
DEF_ASSERT_EQ_INT(i64, "%lld")

DEF_ASSERT_EQ_INT(u8,  "%u")
DEF_ASSERT_EQ_INT(u16, "%u")
DEF_ASSERT_EQ_INT(u32, "%u")
DEF_ASSERT_EQ_INT(u64, "%llu")

DEF_ASSERT_EQ_INT(usize, "%zu")
DEF_ASSERT_EQ_INT(isize, "%zd")

DEF_ASSERT_EQ_INT(uptr, "%" PRIuPTR)
DEF_ASSERT_EQ_INT(iptr, "%" PRIdPTR)

internal_fn void assert_eq_f32(const char* fn, const char* file, int line, const char* ea, const char* eb, f32 a, f32 b) {
    f32 diff = a - b;
    if(diff < 0) {
		diff = -diff;
	}
    if(diff > 1e-6f) {
        TEST_FAIL(fn, file, line, "expected %s=%f, got %s=%f", ea, a, eb, b);
    }
}

internal_fn void assert_eq_f64(const char* fn, const char* file, int line, const char* ea, const char* eb, f64 a, f64 b) {
    f64 diff = a - b;
    if(diff < 0) {
		diff = -diff;
	}
    if(diff > 1e-12) {
        TEST_FAIL(fn, file, line, "expected %s=%f, got %s=%f", ea, a, eb, b);
    }
}

#define ASSERT_EQ_PTR(a, b) \
	do { \
		if((a) != (b)) { \
			TEST_FAIL(__func__, __FILE__, __LINE__, "expected %s=%p, got %s=%p", #b, (b), #a, (a)); \
		} \
	} while(0)

internal_fn void assert_eq_unsupported(const char* fn, const char* file, int line, const char* ea, const char* eb, void* a, void* b) {
    (void)a;
    (void)b;
    TEST_FAIL(fn, file, line, "ASSERT_EQ unsupported type: %s vs %s", ea, eb);
}

#define ASSERT_EQ(a, b) \
    _Generic((a), \
        i8:  assert_eq_i8, \
        i16: assert_eq_i16, \
        i32: assert_eq_i32, \
        i64: assert_eq_i64, \
        u8:  assert_eq_u8, \
        u16: assert_eq_u16, \
        u32: assert_eq_u32, \
        u64: assert_eq_u64, \
        f32: assert_eq_f32, \
        f64: assert_eq_f64, \
		default: assert_eq_unsupported \
    )(__func__, __FILE__, __LINE__, #b, #a, (b), (a))

#define ASSERT_DEATH(stmt) \
    do { \
        pid_t pid = fork(); \
        if(pid == 0) { \
            stmt; \
            _exit(EXIT_SUCCESS); \
        } \
        int status; \
        waitpid(pid, &status, 0); \
        ASSERT_TRUE(WIFSIGNALED(status)); \
        ASSERT_EQ(WTERMSIG(status), SIGABRT); \
    } while(0)

void run_node(const TestNode* node) {
    if(node->setup) {
        node->setup();
    }

    if(node->fn) {
        printf("[RUN ] %s\n", node->name);
        tests_run++;
        node->fn();
        printf("[ OK ] %s\n\n", node->name);
    } else {
        printf("=== %s ===\n", node->name);

        for(int i = 0; i < node->child_count; i++) {
            run_node(&node->children[i]);
        }
    }

    if(node->teardown) {
        node->teardown();
    }
}

#endif // __BASE_FOUNDATION_CORE_TEST__
