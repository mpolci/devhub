// Tiny test helpers: CHECK reports the failing line and counts failures.
#ifndef TEST_H
#define TEST_H

#include <stdio.h>

static int test_failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            test_failures++;                                               \
        }                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                                        \
    do {                                                                                      \
        long va_ = (long)(a), vb_ = (long)(b);                                                \
        if (va_ != vb_) {                                                                     \
            printf("%s:%d: %s = %ld, expected %ld\n", __FILE__, __LINE__, #a, va_, vb_);      \
            test_failures++;                                                                  \
        }                                                                                     \
    } while (0)

#define RUN(fn)             \
    do {                    \
        printf("%s\n", #fn); \
        fn();               \
    } while (0)

#define TEST_RESULT() (test_failures ? (printf("%d failure(s)\n", test_failures), 1) : (printf("ok\n"), 0))

#endif
