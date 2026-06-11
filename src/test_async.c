/* ===================================================================
 * test_async.c – Wrapper for the async persistence stress test suite.
 *
 * The tests live in tests/async/ and are compiled as a separate object
 * (test_persistence.o) linked into the binary when TEST_PERSISTENCE is
 * defined.
 * =================================================================== */

#include "test_async.h"

/* test_async.h includes ../tests/async/test_persistence.h when
 * TEST_PERSISTENCE is defined, which provides all declarations.
 * The implementations live in tests/async/test_persistence.c and
 * are compiled via a custom Makefile rule.
 */
