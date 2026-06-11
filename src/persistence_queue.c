#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "persistence_queue.h"
#include "latency_trace.h"

/* wizlog() and logit() are declared in utility.h / structs.h; pull in
 * just the declarations to avoid dragging heavy dependencies into this
 * translation unit. */
extern void wizlog(int level, const char *format, ...);
extern void logit(const char *filename, const char *format, ...);

struct persistence_event_queue_data
{
  char **events;       /* dynamically allocated: events[i] = malloc(MAX_LEN) */
  int head;
  int tail;
  int count;
  int capacity;        /* current number of slots */
  unsigned long dropped;
  unsigned long resize_count;
};

static persistence_event_queue_data persistence_item_event_queue;
static pthread_mutex_t persistence_item_event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t persistence_item_event_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_t persistence_item_event_worker_thread;
static int persistence_item_event_worker_is_running;
static int persistence_item_event_worker_stop_requested;
static int persistence_item_event_worker_drain_requested;
static persistence_item_event_writer persistence_item_event_worker_writer;
static void *persistence_item_event_worker_context;
static unsigned long persistence_item_event_worker_write_count;
static unsigned long persistence_item_event_worker_failure_count;
static time_t persistence_item_event_worker_last_heartbeat;

static persistence_event_queue_data persistence_scalar_event_queue;
static pthread_mutex_t persistence_scalar_event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t persistence_scalar_event_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_t persistence_scalar_event_worker_thread;
static int persistence_scalar_event_worker_is_running;
static int persistence_scalar_event_worker_stop_requested;
static int persistence_scalar_event_worker_drain_requested;
static persistence_scalar_event_writer persistence_scalar_event_worker_writer;
static void *persistence_scalar_event_worker_context;
static unsigned long persistence_scalar_event_worker_write_count;
static unsigned long persistence_scalar_event_worker_failure_count;
static time_t persistence_scalar_event_worker_last_heartbeat;

/* Large-payload event queue */
static persistence_event_queue_data persistence_large_event_queue;
static pthread_mutex_t persistence_large_event_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t persistence_large_event_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_t persistence_large_event_worker_thread;
static int persistence_large_event_worker_is_running;
static int persistence_large_event_worker_stop_requested;
static int persistence_large_event_worker_drain_requested;
static persistence_scalar_event_writer persistence_large_event_worker_writer;
static void *persistence_large_event_worker_context;
static unsigned long persistence_large_event_worker_write_count;
static unsigned long persistence_large_event_worker_failure_count;
static time_t persistence_large_event_worker_last_heartbeat;

/* ===================================================================
 * Dynamic queue helpers – callers MUST hold the queue mutex.
 * =================================================================== */

/* Allocate internal buffer for 'capacity' event slots.
 * Returns 1 on success, 0 on failure (unchanged on failure). */
static int persistence_queue_alloc(persistence_event_queue_data *q, int capacity)
{
  char **new_events = (char **)calloc(capacity, sizeof(char *));
  if (!new_events) return 0;
  for (int i = 0; i < capacity; i++) {
    new_events[i] = (char *)malloc(PERSISTENCE_EVENT_MAX_LEN);
    if (!new_events[i]) {
      for (int j = 0; j < i; j++) free(new_events[j]);
      free(new_events);
      return 0;
    }
  }
  q->events = new_events;
  q->capacity = capacity;
  return 1;
}

/* Grow the queue to 'new_capacity'. Existing events are migrated.
 * Returns 1 on success, 0 on failure (queue is unchanged). */
static int persistence_queue_grow(persistence_event_queue_data *q, int new_capacity)
{
  char **new_events = (char **)calloc(new_capacity, sizeof(char *));
  if (!new_events) return 0;
  for (int i = 0; i < new_capacity; i++) {
    new_events[i] = (char *)malloc(PERSISTENCE_EVENT_MAX_LEN);
    if (!new_events[i]) {
      for (int j = 0; j < i; j++) free(new_events[j]);
      free(new_events);
      return 0;
    }
  }

  /* Copy existing events preserving ring order */
  int old_count = q->count;
  for (int i = 0; i < old_count; i++) {
    int old_idx = (q->head + i) % q->capacity;
    memcpy(new_events[i], q->events[old_idx], PERSISTENCE_EVENT_MAX_LEN);
  }

  /* Free old storage */
  for (int i = 0; i < q->capacity; i++) free(q->events[i]);
  free(q->events);

  /* Swap in new */
  int old_capacity = q->capacity;
  q->events = new_events;
  q->head = 0;
  q->tail = old_count;
  q->capacity = new_capacity;
  q->resize_count++;

  /* Notify operators and log: queue grew to avoid drops */
  logit("logs/log/status",
    "PERSISTENCE QUEUE RESIZE: capacity %d -> %d (count=%d resize_count=%lu)",
    old_capacity, new_capacity, old_count, q->resize_count);
  wizlog(57,
    "&+R&-LPERSISTENCE QUEUE RESIZE:&n capacity %d -> %d (count=%d resize_count=%lu)",
    old_capacity, new_capacity, old_count, q->resize_count);

  return 1;
}

/* Free all queue memory. Caller must re-initialize before reuse. */
static void persistence_queue_free(persistence_event_queue_data *q)
{
  if (q->events) {
    for (int i = 0; i < q->capacity; i++) {
      if (q->events[i]) free(q->events[i]);
    }
    free(q->events);
    q->events = NULL;
  }
  q->head = 0;
  q->tail = 0;
  q->count = 0;
  q->capacity = 0;
  q->dropped = 0;
  q->resize_count = 0;
}

/* Try to double the queue capacity up to the absolute maximum.
 * Returns 1 if the queue now has room, 0 if it can't grow further. */
static int persistence_queue_auto_grow(persistence_event_queue_data *q, int max_capacity)
{
  int new_cap = q->capacity * 2;
  if (new_cap > max_capacity) new_cap = max_capacity;
  if (new_cap <= q->capacity) return 0;
  return persistence_queue_grow(q, new_cap);
}

static void persistence_item_event_queue_pop_head(void)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;

  if (q->count <= 0)
    return;

  q->head = (q->head + 1) % q->capacity;
  q->count--;
}

int persistence_item_event_queue_enqueue(const char *line)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;
  int ok = 1;

  if (!line || !*line)
    return 0;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);

  /* Lazy init */
  if (!q->events)
    persistence_queue_alloc(q, PERSISTENCE_EVENT_QUEUE_CAPACITY);

  if (q->count >= q->capacity)
  {
    /* Auto-resize: try to double capacity */
    persistence_queue_auto_grow(q, PERSISTENCE_EVENT_QUEUE_MAX_CAPACITY);
  }

  if (q->count >= q->capacity)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], PERSISTENCE_EVENT_MAX_LEN, "%s", line);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&persistence_item_event_queue_cond);
  }

  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return ok;
}

int persistence_item_event_queue_dequeue(char *out, int out_size)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;
  int ok = 0;

  if (!out || out_size <= 0)
    return 0;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  if (q->count > 0)
  {
    snprintf(out, out_size, "%s", q->events[q->head]);
    persistence_item_event_queue_pop_head();
    ok = 1;
  }
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return ok;
}

int persistence_item_event_queue_pending(void)
{
  int pending;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  pending = persistence_item_event_queue.count;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return pending;
}

unsigned long persistence_item_event_queue_dropped(void)
{
  unsigned long dropped;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  dropped = persistence_item_event_queue.dropped;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return dropped;
}

void persistence_item_event_queue_clear_dropped(void)
{
  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  persistence_item_event_queue.dropped = 0;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);
}

void persistence_item_event_queue_reset(void)
{
  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  persistence_queue_free(&persistence_item_event_queue);
  persistence_item_event_worker_write_count = 0;
  persistence_item_event_worker_failure_count = 0;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);
}

static void *persistence_item_event_worker_main(void *unused)
{
  char line[PERSISTENCE_EVENT_MAX_LEN];
  int should_write;
  int write_ok;

  (void) unused;

  while (1)
  {
    should_write = 0;

    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    /* Deadlock-detection heartbeat: advance timestamp under the queue
     * mutex on every iteration. A worker stuck in cond_wait, a blocking
     * MySQL call, or anywhere else without releasing the mutex will
     * leave this stale and be flagged as stuck by
     * persistence_item_event_worker_stuck() on the main thread. */
    persistence_item_event_worker_last_heartbeat = time(NULL);
    while (persistence_item_event_queue.count <= 0 &&
           !persistence_item_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_item_event_queue_cond,
                        &persistence_item_event_queue_mutex);
      /* Refresh heartbeat after waking from cond_wait too. */
      persistence_item_event_worker_last_heartbeat = time(NULL);
    }

    if (persistence_item_event_worker_stop_requested &&
        (!persistence_item_event_worker_drain_requested ||
         persistence_item_event_queue.count <= 0))
    {
      pthread_mutex_unlock(&persistence_item_event_queue_mutex);
      break;
    }

    if (persistence_item_event_queue.count > 0)
    {
      snprintf(line, sizeof(line), "%s",
               persistence_item_event_queue.events[persistence_item_event_queue.head]);
      should_write = 1;
    }
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);

    if (!should_write)
      continue;

    write_ok = persistence_item_event_worker_writer ?
      persistence_item_event_worker_writer(line,
                                           persistence_item_event_worker_context) : 1;

    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    if (write_ok)
    {
      if (persistence_item_event_queue.count > 0 &&
          !strncmp(line,
                   persistence_item_event_queue.events[persistence_item_event_queue.head],
                   PERSISTENCE_EVENT_MAX_LEN))
      {
        persistence_item_event_queue_pop_head();
      }
      persistence_item_event_worker_write_count++;
    }
    else
    {
      persistence_item_event_worker_failure_count++;
    }
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);

    if (!write_ok)
      usleep(PERSISTENCE_WORKER_RETRY_USEC);
  }

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  persistence_item_event_worker_is_running = 0;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return NULL;
}

int persistence_item_event_worker_start(persistence_item_event_writer writer,
                                        void *context)
{
  int result;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  if (persistence_item_event_worker_is_running)
  {
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
    return 1;
  }

  persistence_item_event_worker_writer = writer;
  persistence_item_event_worker_context = context;
  persistence_item_event_worker_stop_requested = 0;
  persistence_item_event_worker_drain_requested = 0;
  persistence_item_event_worker_is_running = 1;
  persistence_item_event_worker_last_heartbeat = time(NULL);
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  result = pthread_create(&persistence_item_event_worker_thread, NULL,
                          persistence_item_event_worker_main, NULL);
  if (result)
  {
    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    persistence_item_event_worker_is_running = 0;
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
    return 0;
  }

  return 1;
}

void persistence_item_event_worker_stop(int drain_remaining)
{
  int was_running;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  was_running = persistence_item_event_worker_is_running;
  persistence_item_event_worker_stop_requested = 1;
  persistence_item_event_worker_drain_requested = drain_remaining ? 1 : 0;
  pthread_cond_signal(&persistence_item_event_queue_cond);
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  if (was_running)
    pthread_join(persistence_item_event_worker_thread, NULL);
}

int persistence_item_event_worker_running(void)
{
  int running;
  int kill_rc;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  running = persistence_item_event_worker_is_running;
  if (running)
  {
    /* Watchdog: the worker thread sets persistence_item_event_worker_is_running=0 under this same
     * mutex before exiting. If the flag is still 1 but the thread is
     * gone (killed by signal, segfault, or pthread_exit that didn't
     * run the cleanup), pthread_kill(tid, 0) returns ESRCH. We clear
     * the flag here so the next producer falls through to the sync
     * path instead of enqueuing into an undrained queue.
     *
     * Note: this does NOT detect deadlocks - a thread stuck in a
     * blocking MySQL call will pass this check. For deadlock
     * detection, a separate heartbeat is needed.
     */
    kill_rc = pthread_kill(persistence_item_event_worker_thread, 0);
    if (kill_rc == ESRCH)
    {
      persistence_item_event_worker_is_running = 0;
      running = 0;
    }
    /* EINVAL: tid is no longer valid (already joined) - shouldn't
     * happen here since the flag is still 1, but ignore it.
     */
  }
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return running;
}

unsigned long persistence_item_event_worker_written(void)
{
  unsigned long count;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  count = persistence_item_event_worker_write_count;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return count;
}

unsigned long persistence_item_event_worker_write_failures(void)
{
  unsigned long count;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  count = persistence_item_event_worker_failure_count;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return count;
}

static void persistence_scalar_event_queue_pop_head(void)
{
  persistence_event_queue_data *q = &persistence_scalar_event_queue;

  if (q->count <= 0)
    return;

  q->head = (q->head + 1) % q->capacity;
  q->count--;
}

static void persistence_large_event_queue_pop_head(void)
{
  persistence_event_queue_data *q = &persistence_large_event_queue;

  if (q->count <= 0)
    return;

  q->head = (q->head + 1) % q->capacity;
  q->count--;
}

int persistence_scalar_event_queue_enqueue(const char *line)
{
  persistence_event_queue_data *q = &persistence_scalar_event_queue;
  int ok = 1;

  if (!line || !*line)
    return 0;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);

  /* Lazy init */
  if (!q->events)
    persistence_queue_alloc(q, PERSISTENCE_EVENT_QUEUE_CAPACITY);

  if (q->count >= q->capacity)
  {
    /* Auto-resize: try to double capacity */
    persistence_queue_auto_grow(q, PERSISTENCE_EVENT_QUEUE_MAX_CAPACITY);
  }

  if (q->count >= q->capacity)
  {
    q->dropped++;
    ok = 0;
    latency_trace_record("scalar_enq_drop", 0, 0);
  }
  else
  {
    snprintf(q->events[q->tail], PERSISTENCE_EVENT_MAX_LEN, "%s", line);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&persistence_scalar_event_queue_cond);
    latency_trace_record("scalar_enq_ok", 0, 0);
  }

  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return ok;
}

int persistence_scalar_event_queue_dequeue(char *out, int out_size)
{
  persistence_event_queue_data *q = &persistence_scalar_event_queue;
  int ok = 0;

  if (!out || out_size <= 0)
    return 0;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  if (q->count > 0)
  {
    snprintf(out, out_size, "%s", q->events[q->head]);
    persistence_scalar_event_queue_pop_head();
    ok = 1;
  }
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return ok;
}

int persistence_scalar_event_queue_pending(void)
{
  int pending;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  pending = persistence_scalar_event_queue.count;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return pending;
}

/*
 * Large-payload event queue
 */
int persistence_large_event_queue_enqueue(const char *line)
{
  persistence_event_queue_data *q = &persistence_large_event_queue;
  int ok = 1;

  if (!line || !*line)
    return 0;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);

  /* Lazy init */
  if (!q->events)
    persistence_queue_alloc(q, PERSISTENCE_LARGE_EVENT_QUEUE_CAPACITY);

  if (q->count >= q->capacity)
  {
    /* Auto-resize: try to double capacity */
    persistence_queue_auto_grow(q, PERSISTENCE_LARGE_EVENT_QUEUE_MAX_CAPACITY);
  }

  if (q->count >= q->capacity)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], PERSISTENCE_LARGE_EVENT_MAX_LEN, "%s", line);
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&persistence_large_event_queue_cond);
  }

  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return ok;
}

int persistence_large_event_queue_dequeue(char *out, int out_size)
{
  persistence_event_queue_data *q = &persistence_large_event_queue;
  int ok = 0;

  if (!out || out_size <= 0)
    return 0;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  if (q->count > 0)
  {
    snprintf(out, out_size, "%s", q->events[q->head]);
    persistence_large_event_queue_pop_head();
    ok = 1;
  }
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return ok;
}

int persistence_large_event_queue_pending(void)
{
  int pending;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  pending = persistence_large_event_queue.count;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return pending;
}

unsigned long persistence_large_event_queue_dropped(void)
{
  unsigned long dropped;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  dropped = persistence_large_event_queue.dropped;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return dropped;
}

void persistence_large_event_queue_clear_dropped(void)
{
  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  persistence_large_event_queue.dropped = 0;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);
}

void persistence_large_event_queue_reset(void)
{
  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  persistence_queue_free(&persistence_large_event_queue);
  persistence_large_event_worker_write_count = 0;
  persistence_large_event_worker_failure_count = 0;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);
}

unsigned long persistence_scalar_event_queue_dropped(void)
{
  unsigned long dropped;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  dropped = persistence_scalar_event_queue.dropped;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return dropped;
}

void persistence_scalar_event_queue_clear_dropped(void)
{
  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  persistence_scalar_event_queue.dropped = 0;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
}

void persistence_scalar_event_queue_reset(void)
{
  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  persistence_queue_free(&persistence_scalar_event_queue);
  persistence_scalar_event_worker_write_count = 0;
  persistence_scalar_event_worker_failure_count = 0;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
}

static void *persistence_scalar_event_worker_main(void *unused)
{
  char line[PERSISTENCE_EVENT_MAX_LEN];
  int should_write;
  int write_ok;

  (void) unused;

  while (1)
  {
    should_write = 0;

    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    /* Deadlock-detection heartbeat: see item worker. */
    persistence_scalar_event_worker_last_heartbeat = time(NULL);
    while (persistence_scalar_event_queue.count <= 0 &&
           !persistence_scalar_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_scalar_event_queue_cond,
                        &persistence_scalar_event_queue_mutex);
      persistence_scalar_event_worker_last_heartbeat = time(NULL);
    }

    if (persistence_scalar_event_worker_stop_requested &&
        (!persistence_scalar_event_worker_drain_requested ||
         persistence_scalar_event_queue.count <= 0))
    {
      pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
      break;
    }

    if (persistence_scalar_event_queue.count > 0)
    {
      snprintf(line, sizeof(line), "%s",
               persistence_scalar_event_queue.events[persistence_scalar_event_queue.head]);
      should_write = 1;
    }
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

    if (!should_write)
      continue;

    write_ok = persistence_scalar_event_worker_writer ?
      persistence_scalar_event_worker_writer(line,
                                             persistence_scalar_event_worker_context) : 1;

    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    if (write_ok)
    {
      if (persistence_scalar_event_queue.count > 0 &&
          !strncmp(line,
                   persistence_scalar_event_queue.events[persistence_scalar_event_queue.head],
                   PERSISTENCE_EVENT_MAX_LEN))
      {
        persistence_scalar_event_queue_pop_head();
      }
      persistence_scalar_event_worker_write_count++;
    }
    else
    {
      persistence_scalar_event_worker_failure_count++;
    }
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

    if (!write_ok)
      usleep(PERSISTENCE_WORKER_RETRY_USEC);
  }

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  persistence_scalar_event_worker_is_running = 0;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return NULL;
}

int persistence_scalar_event_worker_start(persistence_scalar_event_writer writer,
                                          void *context)
{
  int result;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  if (persistence_scalar_event_worker_is_running)
  {
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
    return 1;
  }

  persistence_scalar_event_worker_writer = writer;
  persistence_scalar_event_worker_context = context;
  persistence_scalar_event_worker_stop_requested = 0;
  persistence_scalar_event_worker_drain_requested = 0;
  persistence_scalar_event_worker_is_running = 1;
  persistence_scalar_event_worker_last_heartbeat = time(NULL);
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  result = pthread_create(&persistence_scalar_event_worker_thread, NULL,
                          persistence_scalar_event_worker_main, NULL);
  if (result)
  {
    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    persistence_scalar_event_worker_is_running = 0;
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
    return 0;
  }

  return 1;
}

void persistence_scalar_event_worker_stop(int drain_remaining)
{
  int was_running;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  was_running = persistence_scalar_event_worker_is_running;
  persistence_scalar_event_worker_stop_requested = 1;
  persistence_scalar_event_worker_drain_requested = drain_remaining ? 1 : 0;
  pthread_cond_signal(&persistence_scalar_event_queue_cond);
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  if (was_running)
    pthread_join(persistence_scalar_event_worker_thread, NULL);
}

int persistence_scalar_event_worker_running(void)
{
  int running;
  int kill_rc;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  running = persistence_scalar_event_worker_is_running;
  if (running)
  {
    /* Watchdog: the worker thread sets persistence_scalar_event_worker_is_running=0 under this same
     * mutex before exiting. If the flag is still 1 but the thread is
     * gone (killed by signal, segfault, or pthread_exit that didn't
     * run the cleanup), pthread_kill(tid, 0) returns ESRCH. We clear
     * the flag here so the next producer falls through to the sync
     * path instead of enqueuing into an undrained queue.
     *
     * Note: this does NOT detect deadlocks - a thread stuck in a
     * blocking MySQL call will pass this check. For deadlock
     * detection, a separate heartbeat is needed.
     */
    kill_rc = pthread_kill(persistence_scalar_event_worker_thread, 0);
    if (kill_rc == ESRCH)
    {
      persistence_scalar_event_worker_is_running = 0;
      running = 0;
    }
    /* EINVAL: tid is no longer valid (already joined) - shouldn't
     * happen here since the flag is still 1, but ignore it.
     */
  }
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return running;
}

unsigned long persistence_scalar_event_worker_written(void)
{
  unsigned long count;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  count = persistence_scalar_event_worker_write_count;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return count;
}

unsigned long persistence_scalar_event_worker_write_failures(void)
{
  unsigned long count;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  count = persistence_scalar_event_worker_failure_count;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return count;
}

static void *persistence_large_event_worker_main(void *unused)
{
  char line[PERSISTENCE_LARGE_EVENT_MAX_LEN];
  int should_write;
  int write_ok;

  (void) unused;

  while (1)
  {
    should_write = 0;

    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    /* Deadlock-detection heartbeat: see item worker. */
    persistence_large_event_worker_last_heartbeat = time(NULL);
    while (persistence_large_event_queue.count <= 0 &&
           !persistence_large_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_large_event_queue_cond,
                        &persistence_large_event_queue_mutex);
      persistence_large_event_worker_last_heartbeat = time(NULL);
    }

    if (persistence_large_event_worker_stop_requested &&
        (!persistence_large_event_worker_drain_requested ||
         persistence_large_event_queue.count <= 0))
    {
      pthread_mutex_unlock(&persistence_large_event_queue_mutex);
      break;
    }

    if (persistence_large_event_queue.count > 0)
    {
      snprintf(line, sizeof(line), "%s",
               persistence_large_event_queue.events[persistence_large_event_queue.head]);
      should_write = 1;
    }
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);

    if (!should_write)
      continue;

    write_ok = persistence_large_event_worker_writer ?
      persistence_large_event_worker_writer(line,
                                             persistence_large_event_worker_context) : 1;

    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    if (write_ok)
    {
      if (persistence_large_event_queue.count > 0 &&
          !strncmp(line,
                   persistence_large_event_queue.events[persistence_large_event_queue.head],
                   PERSISTENCE_LARGE_EVENT_MAX_LEN))
      {
        persistence_large_event_queue_pop_head();
      }
      persistence_large_event_worker_write_count++;
    }
    else
    {
      persistence_large_event_worker_failure_count++;
    }
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);

    if (!write_ok)
      usleep(PERSISTENCE_WORKER_RETRY_USEC);
  }

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  persistence_large_event_worker_is_running = 0;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return NULL;
}

int persistence_large_event_worker_start(persistence_scalar_event_writer writer,
                                          void *context)
{
  int result;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  if (persistence_large_event_worker_is_running)
  {
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
    return 1;
  }

  persistence_large_event_worker_writer = writer;
  persistence_large_event_worker_context = context;
  persistence_large_event_worker_stop_requested = 0;
  persistence_large_event_worker_drain_requested = 0;
  persistence_large_event_worker_is_running = 1;
  persistence_large_event_worker_last_heartbeat = time(NULL);
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  result = pthread_create(&persistence_large_event_worker_thread, NULL,
                          persistence_large_event_worker_main, NULL);
  if (result)
  {
    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    persistence_large_event_worker_is_running = 0;
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
    return 0;
  }

  return 1;
}

void persistence_large_event_worker_stop(int drain_remaining)
{
  int was_running;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  was_running = persistence_large_event_worker_is_running;
  persistence_large_event_worker_stop_requested = 1;
  persistence_large_event_worker_drain_requested = drain_remaining ? 1 : 0;
  pthread_cond_signal(&persistence_large_event_queue_cond);
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  if (was_running)
    pthread_join(persistence_large_event_worker_thread, NULL);
}

int persistence_large_event_worker_running(void)
{
  int running;
  int kill_rc;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  running = persistence_large_event_worker_is_running;
  if (running)
  {
    /* Watchdog: the worker thread sets persistence_large_event_worker_is_running=0 under this same
     * mutex before exiting. If the flag is still 1 but the thread is
     * gone (killed by signal, segfault, or pthread_exit that didn't
     * run the cleanup), pthread_kill(tid, 0) returns ESRCH. We clear
     * the flag here so the next producer falls through to the sync
     * path instead of enqueuing into an undrained queue.
     *
     * Note: this does NOT detect deadlocks - a thread stuck in a
     * blocking MySQL call will pass this check. For deadlock
     * detection, a separate heartbeat is needed.
     */
    kill_rc = pthread_kill(persistence_large_event_worker_thread, 0);
    if (kill_rc == ESRCH)
    {
      persistence_large_event_worker_is_running = 0;
      running = 0;
    }
    /* EINVAL: tid is no longer valid (already joined) - shouldn't
     * happen here since the flag is still 1, but ignore it.
     */
  }
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return running;
}

unsigned long persistence_large_event_worker_written(void)
{
  unsigned long count;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  count = persistence_large_event_worker_write_count;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return count;
}

unsigned long persistence_large_event_worker_write_failures(void)
{
  unsigned long count;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  count = persistence_large_event_worker_failure_count;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return count;
}

int persistence_item_event_worker_heartbeat_age(void)
{
  time_t last;
  int age;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  last = persistence_item_event_worker_last_heartbeat;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  if (last == 0)
    return -1;

  age = (int)(time(NULL) - last);
  return age >= 0 ? age : 0;
}

int persistence_scalar_event_worker_heartbeat_age(void)
{
  time_t last;
  int age;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  last = persistence_scalar_event_worker_last_heartbeat;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  if (last == 0)
    return -1;

  age = (int)(time(NULL) - last);
  return age >= 0 ? age : 0;
}

int persistence_large_event_worker_heartbeat_age(void)
{
  time_t last;
  int age;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  last = persistence_large_event_worker_last_heartbeat;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  if (last == 0)
    return -1;

  age = (int)(time(NULL) - last);
  return age >= 0 ? age : 0;
}

void persistence_item_event_worker_heartbeat_set(time_t timestamp)
{
  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  persistence_item_event_worker_last_heartbeat = timestamp;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);
}

void persistence_scalar_event_worker_heartbeat_set(time_t timestamp)
{
  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  persistence_scalar_event_worker_last_heartbeat = timestamp;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
}

void persistence_large_event_worker_heartbeat_set(time_t timestamp)
{
  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  persistence_large_event_worker_last_heartbeat = timestamp;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);
}

void persistence_queue_latency_dump(void)
{
  FILE *f = fopen("/durismud/logs/latency_trace.log", "a");
  if (!f) return;
  latency_trace_dump(f);
  fclose(f);
}

void persistence_queue_latency_reset(void)
{
  latency_trace_reset();
}
