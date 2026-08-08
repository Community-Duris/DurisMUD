#ifndef __PERSISTENCE_QUEUE_H_INCLUDED__
#define __PERSISTENCE_QUEUE_H_INCLUDED__

#define PERSISTENCE_EVENT_QUEUE_CAPACITY 4096
#define PERSISTENCE_EVENT_QUEUE_MAX_CAPACITY 131072
#define PERSISTENCE_EVENT_MAX_LEN 1024
#define PERSISTENCE_FLUSH_BATCH_MAX 256
#define PERSISTENCE_WORKER_RETRY_USEC 100000

/* Deadlock-detection heartbeat: if a worker hasn't advanced its
 * timestamp within this many seconds while still flagged
 * running, the main-thread watchdog flags it as stuck. */
#define PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS 30

/* Large-payload event queue — for events that exceed the 1024-byte limit
 * (e.g. pkill_info with full equipment list and player log).
 * 64 slots x 128KB = 8MB total.
 */
#define PERSISTENCE_LARGE_EVENT_QUEUE_CAPACITY 64
#define PERSISTENCE_LARGE_EVENT_QUEUE_MAX_CAPACITY 2048
#define PERSISTENCE_LARGE_EVENT_MAX_LEN 131072

typedef int (*persistence_item_event_writer)(const char *line, void *context);
typedef int (*persistence_scalar_event_writer)(const char *line, void *context);

int persistence_item_event_queue_enqueue(const char *line);
int persistence_item_event_queue_dequeue(char *out, int out_size);
int persistence_item_event_queue_pending(void);
unsigned long persistence_item_event_queue_dropped(void);
void persistence_item_event_queue_clear_dropped(void);
void persistence_item_event_queue_reset(void);

int persistence_item_event_worker_start(persistence_item_event_writer writer,
                                        void *context);
int persistence_item_event_worker_stop(int drain_remaining);
int persistence_item_event_worker_running(void);
int persistence_item_event_worker_stop_pending(void);
unsigned long persistence_item_event_worker_written(void);
unsigned long persistence_item_event_worker_write_failures(void);

int persistence_scalar_event_queue_enqueue(const char *line);
int persistence_scalar_event_queue_dequeue(char *out, int out_size);
int persistence_scalar_event_queue_pending(void);
unsigned long persistence_scalar_event_queue_dropped(void);
void persistence_scalar_event_queue_clear_dropped(void);
void persistence_scalar_event_queue_reset(void);

int persistence_scalar_event_worker_start(persistence_scalar_event_writer writer,
                                          void *context);
int persistence_scalar_event_worker_stop(int drain_remaining);
int persistence_scalar_event_worker_running(void);
int persistence_scalar_event_worker_stop_pending(void);
unsigned long persistence_scalar_event_worker_written(void);
unsigned long persistence_scalar_event_worker_write_failures(void);

/* Large-payload event queue */
int persistence_large_event_queue_enqueue(const char *line);
int persistence_large_event_queue_dequeue(char *out, int out_size);
int persistence_large_event_queue_pending(void);
unsigned long persistence_large_event_queue_dropped(void);
void persistence_large_event_queue_clear_dropped(void);
void persistence_large_event_queue_reset(void);

int persistence_large_event_worker_start(persistence_scalar_event_writer writer,
                                          void *context);
int persistence_large_event_worker_stop(int drain_remaining);
int persistence_large_event_worker_running(void);
int persistence_large_event_worker_stop_pending(void);
unsigned long persistence_large_event_worker_written(void);
unsigned long persistence_large_event_worker_write_failures(void);

int persistence_item_event_worker_stuck(int threshold_secs);
int persistence_scalar_event_worker_stuck(int threshold_secs);
int persistence_large_event_worker_stuck(int threshold_secs);

/* Heartbeat age: seconds since worker last updated its heartbeat.
 * Returns -1 if the worker was never started (last_heartbeat == 0).
 * Used by the main-thread watchdog to decide whether to auto-restart. */
int persistence_item_event_worker_heartbeat_age(void);
int persistence_scalar_event_worker_heartbeat_age(void);
int persistence_large_event_worker_heartbeat_age(void);

/* Test helpers – set the heartbeat timestamp to simulate a stale worker.
 * These can be used by tests to avoid waiting for the real timeout.
 * Pass get_time() - age_secs to set the heartbeat to `age_secs` seconds ago. */
void persistence_item_event_worker_heartbeat_set(time_t timestamp);
void persistence_scalar_event_worker_heartbeat_set(time_t timestamp);
void persistence_large_event_worker_heartbeat_set(time_t timestamp);

/* Cross-TU latency-trace dump: if latency_trace.h is included in multiple
 * translation units, each has its own static ring buffer.  Call this from
 * the main-thread periodic path (e.g. persistence_worker_heartbeat_check)
 * to dump the scalar-event-queue's trace data to the shared log file. */
void persistence_queue_latency_dump(void);

/* Cross-TU latency-trace reset: resets the persistence_queue.c static ring
 * buffer.  Call before boot-time tests to get clean trace data. */
void persistence_queue_latency_reset(void);

/* SQL string escaping: doubles apostrophes/backslashes, replaces
 * pipes/newlines/carriage-returns with spaces. Returns the output buffer.
 * Returns empty string if buf is NULL or buf_size <= 0.
 * Returns "none" if input is NULL. */
const char *persistence_sql_escape_field(const char *in, char *buf,
                                         int buf_size);
#endif
