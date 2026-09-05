#ifndef TI_C54X_TEST_HARNESS_H
#define TI_C54X_TEST_HARNESS_H

/* Module contract
 * Owns: dependency-free test checks and deterministic failure reporting.
 * Does not own: production assertions, test fixtures, or test-case discovery.
 * Tests must report failures even when
 * compiled with -DNDEBUG; no behavior depends on <assert.h> or an external
 * unit-test framework.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static inline void
ti_c54x_test_fail(const char *file,
                  int line,
                  const char *expression)
{
    (void)fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expression);
    exit(EXIT_FAILURE);
}

static inline void
ti_c54x_test_fail_u64(const char *file,
                      int line,
                      const char *actual_expression,
                      const char *expected_expression,
                      uint64_t actual,
                      uint64_t expected)
{
    (void)fprintf(stderr,
                  "%s:%d: check failed: %s == %s (actual=%" PRIu64
                  ", expected=%" PRIu64 ")\n",
                  file,
                  line,
                  actual_expression,
                  expected_expression,
                  actual,
                  expected);
    exit(EXIT_FAILURE);
}

#define TI_C54X_TEST_CHECK(condition)                                             \
    do {                                                                            \
        if (!(condition)) {                                                        \
            ti_c54x_test_fail(__FILE__, __LINE__, #condition);                     \
        }                                                                           \
    } while (0)

#define TI_C54X_TEST_CHECK_U64(actual_expression, expected_expression)             \
    do {                                                                            \
        const uint64_t ti_c54x_test_actual = (uint64_t)(actual_expression);        \
        const uint64_t ti_c54x_test_expected = (uint64_t)(expected_expression);    \
        if (ti_c54x_test_actual != ti_c54x_test_expected) {                         \
            ti_c54x_test_fail_u64(__FILE__,                                        \
                                  __LINE__,                                        \
                                  #actual_expression,                              \
                                  #expected_expression,                            \
                                  ti_c54x_test_actual,                             \
                                  ti_c54x_test_expected);                           \
        }                                                                           \
    } while (0)

#endif
