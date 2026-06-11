/* ===================================================================
 * test_async.h – Thin wrapper that includes the async persistence
 * test header from tests/async/.
 *
 * Include this from testcmd.c or comm.c to access test functions.
 * =================================================================== */

#ifndef __TEST_ASYNC_WRAPPER_H__
#define __TEST_ASYNC_WRAPPER_H__

#ifdef TEST_PERSISTENCE
#include "../tests/async/test_persistence.h"
#else
/* Stubs when test code is not compiled */
static inline void test_persistence_run_all(void) {}
static inline int  test_persistence_run_one(const char *n) { (void)n; return 0; }
static inline void test_persistence_print_summary(void) {}
static inline void test_persistence_reset(void) {}
#endif

#endif /* __TEST_ASYNC_WRAPPER_H__ */
