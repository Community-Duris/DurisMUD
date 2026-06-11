/* ===================================================================
 * latency_trace.h – Minimal latency tracing ring buffer
 *
 * Records wall-clock duration (microseconds) for named sections in
 * the game loop.  Data is stored in a fixed-size ring buffer and can
 * be dumped to the status log for analysis.
 *
 * Integration:
 *   1. LATENCY_TRACE("name") { ... }   — scope-based block tracer
 *   2. latency_trace_dump()            — write summary to log
 *   3. latency_trace_reset()           — clear buffer
 *
 * No dynamic allocation, no new files, no external dependencies.
 * =================================================================== */

#ifndef __LATENCY_TRACE_H__
#define __LATENCY_TRACE_H__

#include <time.h>
#include <stdio.h>
#include <pthread.h>

/* ======================== Tunables ======================== */

#ifndef LATENCY_TRACE_MAX_SAMPLES
#define LATENCY_TRACE_MAX_SAMPLES  4096
#endif

#ifndef LATENCY_TRACE_ENABLED
#define LATENCY_TRACE_ENABLED      1
#endif

/* ======================== Data ======================== */

typedef struct {
    const char *name;         /* section name (pointer to string literal) */
    long         duration_us; /* elapsed microseconds */
    long         tick;        /* game pulse when recorded */
} latency_entry;

static latency_entry _latency_buf[LATENCY_TRACE_MAX_SAMPLES];
static int           _latency_head  = 0;
static int           _latency_count = 0;
static pthread_mutex_t _latency_mutex = PTHREAD_MUTEX_INITIALIZER;

/* per-section accumulators for summary */
typedef struct {
    const char *name;
    long         min_us;
    long         max_us;
    long         total_us;
    int          count;
} _latency_section;

#define _LATENCY_MAX_SECTIONS 32
static _latency_section _latency_sections[_LATENCY_MAX_SECTIONS];
static int              _latency_nsections = 0;

/* ======================== Internal helpers ======================== */

static int _latency_find_section(const char *name)
{
    for (int i = 0; i < _latency_nsections; i++)
        if (_latency_sections[i].name == name)
            return i;
    return -1;
}

static void _latency_update_section(const char *name, long us)
{
    int idx = _latency_find_section(name);
    if (idx < 0 && _latency_nsections < _LATENCY_MAX_SECTIONS) {
        idx = _latency_nsections++;
        _latency_sections[idx].name    = name;
        _latency_sections[idx].min_us   = us;
        _latency_sections[idx].max_us   = us;
        _latency_sections[idx].total_us = us;
        _latency_sections[idx].count    = 1;
        return;
    }
    if (idx >= 0) {
        if (us < _latency_sections[idx].min_us)
            _latency_sections[idx].min_us = us;
        if (us > _latency_sections[idx].max_us)
            _latency_sections[idx].max_us = us;
        _latency_sections[idx].total_us += us;
        _latency_sections[idx].count++;
    }
}

/* ======================== Public API ======================== */

static inline void latency_trace_record(const char *name, long duration_us, long tick)
{
#if LATENCY_TRACE_ENABLED
    pthread_mutex_lock(&_latency_mutex);
    int idx = _latency_head;
    _latency_buf[idx].name        = name;
    _latency_buf[idx].duration_us = duration_us;
    _latency_buf[idx].tick        = tick;
    _latency_head = (idx + 1) % LATENCY_TRACE_MAX_SAMPLES;
    if (_latency_count < LATENCY_TRACE_MAX_SAMPLES)
        _latency_count++;
    _latency_update_section(name, duration_us);
    pthread_mutex_unlock(&_latency_mutex);
#else
    (void)name; (void)duration_us; (void)tick;
#endif
}

static inline void latency_trace_reset(void)
{
#if LATENCY_TRACE_ENABLED
    pthread_mutex_lock(&_latency_mutex);
    _latency_head      = 0;
    _latency_count     = 0;
    _latency_nsections = 0;
    pthread_mutex_unlock(&_latency_mutex);
#endif
}

static inline void latency_trace_dump(FILE *output)
{
#if LATENCY_TRACE_ENABLED
    if (!output) return;
    pthread_mutex_lock(&_latency_mutex);

    fprintf(output, "\n===== LATENCY TRACE SUMMARY =====\n");
    fprintf(output, "%-30s %8s %8s %10s %8s\n",
            "Section", "min(us)", "max(us)", "avg(us)", "samples");

    /* Sort by total descending */
    for (int i = 0; i < _latency_nsections; i++) {
        for (int j = i + 1; j < _latency_nsections; j++) {
            if (_latency_sections[j].total_us > _latency_sections[i].total_us) {
                _latency_section tmp = _latency_sections[i];
                _latency_sections[i] = _latency_sections[j];
                _latency_sections[j] = tmp;
            }
        }
    }

    for (int i = 0; i < _latency_nsections; i++) {
        _latency_section *s = &_latency_sections[i];
        long avg_us = s->count > 0 ? s->total_us / s->count : 0;
        fprintf(output, "%-30s %8ld %8ld %10ld %8d\n",
                s->name, s->min_us, s->max_us, avg_us, s->count);
    }

    /* Top-10 worst individual samples */
    fprintf(output, "\n--- Top-10 worst individual samples ---\n");
    fprintf(output, "%-30s %8s %8s\n", "Section", "us", "tick");

    /* Find top 10 from ring buffer - scan entire buffer */
    int top_count = 0;
    int top_idx[10];
    for (int i = 0; i < top_count; i++) top_idx[i] = -1;

    int n = _latency_count < LATENCY_TRACE_MAX_SAMPLES ? _latency_count : LATENCY_TRACE_MAX_SAMPLES;
    for (int i = 0; i < n; i++) {
        long us = _latency_buf[i].duration_us;
        /* Insert into top-10 if larger than current 10th */
        if (top_count < 10) {
            top_idx[top_count++] = i;
            /* bubble up */
            for (int k = top_count - 1; k > 0; k--) {
                if (_latency_buf[top_idx[k]].duration_us > _latency_buf[top_idx[k-1]].duration_us) {
                    int t = top_idx[k]; top_idx[k] = top_idx[k-1]; top_idx[k-1] = t;
                }
            }
        } else if (us > _latency_buf[top_idx[9]].duration_us) {
            top_idx[9] = i;
            /* bubble up */
            for (int k = 9; k > 0; k--) {
                if (_latency_buf[top_idx[k]].duration_us > _latency_buf[top_idx[k-1]].duration_us) {
                    int t = top_idx[k]; top_idx[k] = top_idx[k-1]; top_idx[k-1] = t;
                }
            }
        }
    }

    for (int i = 0; i < top_count && i < 10; i++) {
        int idx = top_idx[i];
        fprintf(output, "%-30s %8ld %8ld\n",
                _latency_buf[idx].name,
                _latency_buf[idx].duration_us,
                _latency_buf[idx].tick);
    }

    fprintf(output, "===== END LATENCY TRACE =====\n\n");
    pthread_mutex_unlock(&_latency_mutex);
#else
    (void)output;
#endif
}

/* ======================== Scope-based convenience macro ========================
 *
 * Usage:  LATENCY_TRACE("name") { ... code to time ... }
 *
 * Executes the body exactly once, recording wall-clock duration.
 * Uses _lt_end.tv_sec as a sentinel: zero-initialized before first iteration,
 * set to non-zero by clock_gettime in the increment, so the loop exits after
 * the first complete iteration.
 */

#define LATENCY_TRACE(name) \
    for (struct timespec _lt_start, _lt_end = {0}; \
         !_lt_end.tv_sec && (clock_gettime(CLOCK_MONOTONIC, &_lt_start), 1); \
         clock_gettime(CLOCK_MONOTONIC, &_lt_end), \
         ({ long _us = (_lt_end.tv_sec - _lt_start.tv_sec) * 1000000L + \
                       (_lt_end.tv_nsec - _lt_start.tv_nsec) / 1000L; \
            extern int pulse; \
            latency_trace_record(name, _us, pulse); }))

#endif /* __LATENCY_TRACE_H__ */
