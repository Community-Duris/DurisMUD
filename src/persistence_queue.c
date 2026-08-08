#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#ifndef __NO_MYSQL__
#include <mysql.h>
#endif

#include "persistence_queue.h"
#include "latency_trace.h"

/* wizlog() and logit() are declared in utility.h / structs.h; pull in
 * just the declarations to avoid dragging heavy dependencies into this
 * translation unit. */
extern void wizlog(int level, const char *format, ...);
extern void logit(const char *filename, const char *format, ...);
extern time_t get_time(void);

#define PERSISTENCE_WORKER_STOP_JOIN_TIMEOUT_SECS 2

#ifndef __NO_MYSQL__
static int persistence_worker_mysql_thread_init(const char *worker_name)
{
  if (mysql_thread_init() != 0)
  {
    logit("logs/log/status",
          "%s: mysql_thread_init failed; worker will not start",
          worker_name);
    return 0;
  }

  return 1;
}

static void persistence_worker_mysql_thread_end(void)
{
  mysql_thread_end();
}
#else
static int persistence_worker_mysql_thread_init(const char *worker_name)
{
  (void) worker_name;
  return 1;
}

static void persistence_worker_mysql_thread_end(void)
{
}
#endif

struct persistence_event_queue_data
{
  char **events;       /* dynamically allocated: events[i] = malloc(MAX_LEN) */
  unsigned long long *generations; /* stable identity for each occupied slot */
  unsigned long long next_generation;
  int slot_size;       /* bytes allocated for each events[i] slot */
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
static int persistence_item_event_worker_in_write;
static int persistence_item_event_worker_stop_pending_flag;

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
static int persistence_scalar_event_worker_in_write;
static int persistence_scalar_event_worker_stop_pending_flag;

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
static int persistence_large_event_worker_in_write;
static int persistence_large_event_worker_stop_pending_flag;

static int persistence_worker_timed_join(pthread_t thread)
{
  struct timespec deadline;

  if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    return pthread_join(thread, NULL) == 0;

  deadline.tv_sec += PERSISTENCE_WORKER_STOP_JOIN_TIMEOUT_SECS;
  return pthread_timedjoin_np(thread, NULL, &deadline) == 0;
}

/* ===================================================================
 * Dynamic queue helpers – callers MUST hold the queue mutex.
 * =================================================================== */

/* Allocate internal buffer for 'capacity' event slots.
 * Returns 1 on success, 0 on failure (unchanged on failure). */
static int persistence_queue_alloc(persistence_event_queue_data *q, int capacity, int slot_size)
{
  char **new_events = (char **)calloc(capacity, sizeof(char *));
  unsigned long long *new_generations;

  if (!new_events) return 0;
  new_generations = (unsigned long long *)calloc(capacity, sizeof(*new_generations));
  if (!new_generations) {
    free(new_events);
    return 0;
  }
  for (int i = 0; i < capacity; i++) {
    new_events[i] = (char *)malloc(slot_size);
    if (!new_events[i]) {
      for (int j = 0; j < i; j++) free(new_events[j]);
      free(new_generations);
      free(new_events);
      return 0;
    }
  }
  q->events = new_events;
  q->generations = new_generations;
  q->next_generation = 1;
  q->slot_size = slot_size;
  q->capacity = capacity;
  return 1;
}

/* Grow the queue to 'new_capacity'. Existing events are migrated.
 * Returns 1 on success, 0 on failure (queue is unchanged). */
static int persistence_queue_grow(persistence_event_queue_data *q, int new_capacity)
{
  char **new_events = (char **)calloc(new_capacity, sizeof(char *));
  unsigned long long *new_generations;

  if (!new_events) return 0;
  new_generations = (unsigned long long *)calloc(new_capacity, sizeof(*new_generations));
  if (!new_generations) {
    free(new_events);
    return 0;
  }
  for (int i = 0; i < new_capacity; i++) {
    new_events[i] = (char *)malloc(q->slot_size);
    if (!new_events[i]) {
      for (int j = 0; j < i; j++) free(new_events[j]);
      free(new_generations);
      free(new_events);
      return 0;
    }
  }

  /* Copy existing events preserving ring order and stable identity. */
  int old_count = q->count;
  for (int i = 0; i < old_count; i++) {
    int old_idx = (q->head + i) % q->capacity;
    memcpy(new_events[i], q->events[old_idx], q->slot_size);
    new_generations[i] = q->generations[old_idx];
  }

  /* Free old storage */
  for (int i = 0; i < q->capacity; i++) free(q->events[i]);
  free(q->events);
  free(q->generations);

  /* Swap in new */
  int old_capacity = q->capacity;
  q->events = new_events;
  q->generations = new_generations;
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
  free(q->generations);
  q->generations = NULL;
  q->next_generation = 1;
  q->head = 0;
  q->tail = 0;
  q->count = 0;
  q->capacity = 0;
  q->slot_size = 0;
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

static int persistence_queue_line_too_long(const persistence_event_queue_data *q, const char *line)
{
  if (!q || !line)
    return 1;

  return (int)strlen(line) >= q->slot_size;
}

static unsigned long long persistence_queue_next_generation(persistence_event_queue_data *q)
{
  unsigned long long generation = q->next_generation++;

  if (generation == 0)
    generation = q->next_generation++;
  return generation;
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
  {
    if (!persistence_queue_alloc(q, PERSISTENCE_EVENT_QUEUE_CAPACITY, PERSISTENCE_EVENT_MAX_LEN))
    {
      logit("logs/log/debug", "persistence_item_event_queue_enqueue: failed to allocate queue\n");
      pthread_mutex_unlock(&persistence_item_event_queue_mutex);
      return 0;
    }
  }

  if (persistence_queue_line_too_long(q, line))
  {
    int routed = 0;

    if (strlen(line) < PERSISTENCE_LARGE_EVENT_MAX_LEN)
      routed = persistence_large_event_queue_enqueue(line);

    if (routed)
    {
      pthread_mutex_unlock(&persistence_item_event_queue_mutex);
      return 1;
    }

    logit("logs/log/debug", "persistence_item_event_queue_enqueue: line too long for small queue slot; large queue unavailable\n");
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
    return 0;
  }

  if (q->count >= q->capacity)
  {
    /* Auto-resize: try to double capacity */
    if (!persistence_queue_auto_grow(q, PERSISTENCE_EVENT_QUEUE_MAX_CAPACITY))
    {
      logit("logs/log/debug", "persistence_item_event_queue_enqueue: failed to grow queue\n");
      q->dropped++;
      pthread_mutex_unlock(&persistence_item_event_queue_mutex);
      return 0;
    }
  }

  if (q->count >= q->capacity)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], q->slot_size, "%s", line);
    q->generations[q->tail] = persistence_queue_next_generation(q);
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
  unsigned long long persistence_item_event_worker_generation = 0;
  int should_write;
  int write_ok;

  (void) unused;

  if (!persistence_worker_mysql_thread_init("item persistence worker"))
  {
    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    /* Creation succeeded, so retain ownership until worker_stop() joins. */
    persistence_item_event_worker_stop_pending_flag = 1;
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
    return NULL;
  }

  while (1)
  {
    should_write = 0;

    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    /* Deadlock-detection heartbeat: advance timestamp under the queue
     * mutex on every iteration. A worker stuck in cond_wait, a blocking
     * MySQL call, or anywhere else without releasing the mutex will
     * leave this stale and be flagged as stuck by
     * persistence_item_event_worker_stuck() on the main thread. */
    persistence_item_event_worker_last_heartbeat = get_time();
    while (persistence_item_event_queue.count <= 0 &&
           !persistence_item_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_item_event_queue_cond,
                        &persistence_item_event_queue_mutex);
      /* Refresh heartbeat after waking from cond_wait too. */
      persistence_item_event_worker_last_heartbeat = get_time();
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
      persistence_item_event_worker_generation =
          persistence_item_event_queue.generations[persistence_item_event_queue.head];
      should_write = 1;
    }
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);

    if (!should_write)
      continue;

    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    persistence_item_event_worker_in_write = 1;
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);

    write_ok = persistence_item_event_worker_writer ?
      persistence_item_event_worker_writer(line,
                                           persistence_item_event_worker_context) : 1;

    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    persistence_item_event_worker_in_write = 0;
    if (write_ok)
    {
      if (persistence_item_event_queue.count > 0 &&
          persistence_item_event_queue.generations[
              persistence_item_event_queue.head] ==
              persistence_item_event_worker_generation)
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

  persistence_worker_mysql_thread_end();

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

  if (persistence_item_event_worker_stop_pending_flag)
  {
    /* Quarantine is cleared only by the stop path after a successful join. */
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
    return 0;
  }

  persistence_item_event_worker_writer = writer;
  persistence_item_event_worker_context = context;
  persistence_item_event_worker_stop_requested = 0;
  persistence_item_event_worker_drain_requested = 0;
  persistence_item_event_worker_is_running = 1;
  persistence_item_event_worker_last_heartbeat = get_time();
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

int persistence_item_event_worker_stop(int drain_remaining)
{
  int was_running;
  int stuck;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  was_running = persistence_item_event_worker_is_running ||
                persistence_item_event_worker_stop_pending_flag;
  stuck = was_running &&
          persistence_item_event_worker_last_heartbeat != 0 &&
          (int)(get_time() - persistence_item_event_worker_last_heartbeat) >=
            PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS;
  persistence_item_event_worker_stop_pending_flag = was_running ? 1 : persistence_item_event_worker_stop_pending_flag;
  if (stuck)
    persistence_item_event_worker_is_running = 0;
  persistence_item_event_worker_stop_requested = 1;
  persistence_item_event_worker_drain_requested = drain_remaining ? 1 : 0;
  pthread_cond_signal(&persistence_item_event_queue_cond);
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  if (was_running)
  {
    if (!persistence_worker_timed_join(persistence_item_event_worker_thread))
    {
      logit("logs/log/debug",
            "PERSISTENCE: domain=item_event owner=worker action=stop_timeout detail=worker stop did not complete within %d sec; keeping stop gate set",
            PERSISTENCE_WORKER_STOP_JOIN_TIMEOUT_SECS);
      return 0;
    }

    pthread_mutex_lock(&persistence_item_event_queue_mutex);
    persistence_item_event_worker_stop_pending_flag = 0;
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
  }
  return 1;
}

int persistence_item_event_worker_running(void)
{
  int running;
  int kill_rc;
  time_t last;
  int age;

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
     * A thread that is still alive but has not advanced its heartbeat
     * for too long is also treated as unavailable so the main thread can
     * fail closed and fall back to synchronous persistence instead of
     * waiting forever on a wedged worker.
     */
    if (persistence_item_event_worker_stop_pending_flag)
    {
      /* Only a successful join proves that this generation is reaped. */
      running = 0;
    }
    else if (persistence_item_event_worker_in_write)
    {
      running = 1;
    }
    else
    {
      kill_rc = pthread_kill(persistence_item_event_worker_thread, 0);
      if (kill_rc == ESRCH)
      {
        /* ESRCH is not a safe reap proof. Quarantine so start() will not
         * create a replacement generation until worker_stop() joins. */
        persistence_item_event_worker_stop_pending_flag = 1;
        running = 0;
      }
      else
      {
        last = persistence_item_event_worker_last_heartbeat;
        age = last ? (int)(get_time() - last) : -1;
        if (age >= PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS)
        {
          /* Heartbeat staleness is not proof that the pthread exited.  Keep
           * the generation quarantined so start() cannot create a duplicate
           * while the old worker may still be writing.  worker_stop() owns
           * the bounded join and clears this gate only after a successful
           * join. */
          persistence_item_event_worker_stop_pending_flag = 1;
          running = 0;
        }
      }
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
  {
    if (!persistence_queue_alloc(q, PERSISTENCE_EVENT_QUEUE_CAPACITY, PERSISTENCE_EVENT_MAX_LEN))
    {
      logit("logs/log/debug", "persistence_scalar_event_queue_enqueue: failed to allocate queue\n");
      pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
      return 0;
    }
  }

  if (persistence_queue_line_too_long(q, line))
  {
    int routed = 0;

    if (strlen(line) < PERSISTENCE_LARGE_EVENT_MAX_LEN)
      routed = persistence_large_event_queue_enqueue(line);

    if (routed)
    {
      pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
      return 1;
    }

    logit("logs/log/debug", "persistence_scalar_event_queue_enqueue: line too long for small queue slot; large queue unavailable\n");
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
    return 0;
  }

  if (q->count >= q->capacity)
  {
    /* Auto-resize: try to double capacity */
    if (!persistence_queue_auto_grow(q, PERSISTENCE_EVENT_QUEUE_MAX_CAPACITY))
    {
      logit("logs/log/debug", "persistence_scalar_event_queue_enqueue: failed to grow queue\n");
      q->dropped++;
      ok = 0;
      latency_trace_record("scalar_enq_drop", 0, 0);
    }
  }

  if (q->count >= q->capacity)
  {
    q->dropped++;
    ok = 0;
    latency_trace_record("scalar_enq_drop", 0, 0);
  }
  else
  {
    snprintf(q->events[q->tail], q->slot_size, "%s", line);
    q->generations[q->tail] = persistence_queue_next_generation(q);
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
  {
    if (!persistence_queue_alloc(q, PERSISTENCE_LARGE_EVENT_QUEUE_CAPACITY, PERSISTENCE_LARGE_EVENT_MAX_LEN))
    {
      logit("logs/log/debug", "persistence_large_event_queue_enqueue: failed to allocate queue\n");
      pthread_mutex_unlock(&persistence_large_event_queue_mutex);
      return 0;
    }
  }

  if (persistence_queue_line_too_long(q, line))
  {
    logit("logs/log/debug", "persistence_large_event_queue_enqueue: line too long for queue slot\n");
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
    return 0;
  }

  if (q->count >= q->capacity)
  {
    /* Auto-resize: try to double capacity */
    if (!persistence_queue_auto_grow(q, PERSISTENCE_LARGE_EVENT_QUEUE_MAX_CAPACITY))
    {
      logit("logs/log/debug", "persistence_large_event_queue_enqueue: failed to grow queue\n");
      q->dropped++;
      pthread_mutex_unlock(&persistence_large_event_queue_mutex);
      return 0;
    }
  }

  if (q->count >= q->capacity)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], q->slot_size, "%s", line);
    q->generations[q->tail] = persistence_queue_next_generation(q);
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
  unsigned long long persistence_scalar_event_worker_generation = 0;
  int should_write;
  int write_ok;

  (void) unused;

  if (!persistence_worker_mysql_thread_init("scalar persistence worker"))
  {
    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    persistence_scalar_event_worker_stop_pending_flag = 1;
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
    return NULL;
  }

  while (1)
  {
    should_write = 0;

    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    /* Deadlock-detection heartbeat: see item worker. */
    persistence_scalar_event_worker_last_heartbeat = get_time();
    while (persistence_scalar_event_queue.count <= 0 &&
           !persistence_scalar_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_scalar_event_queue_cond,
                        &persistence_scalar_event_queue_mutex);
      persistence_scalar_event_worker_last_heartbeat = get_time();
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
      persistence_scalar_event_worker_generation =
          persistence_scalar_event_queue.generations[persistence_scalar_event_queue.head];
      should_write = 1;
    }
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

    if (!should_write)
      continue;

    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    persistence_scalar_event_worker_in_write = 1;
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

    write_ok = persistence_scalar_event_worker_writer ?
      persistence_scalar_event_worker_writer(line,
                                             persistence_scalar_event_worker_context) : 1;

    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    persistence_scalar_event_worker_in_write = 0;
    if (write_ok)
    {
      if (persistence_scalar_event_queue.count > 0 &&
          persistence_scalar_event_queue.generations[
              persistence_scalar_event_queue.head] ==
              persistence_scalar_event_worker_generation)
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

  persistence_worker_mysql_thread_end();

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

  if (persistence_scalar_event_worker_stop_pending_flag)
  {
    /* Quarantine is cleared only by the stop path after a successful join. */
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
    return 0;
  }

  persistence_scalar_event_worker_writer = writer;
  persistence_scalar_event_worker_context = context;
  persistence_scalar_event_worker_stop_requested = 0;
  persistence_scalar_event_worker_drain_requested = 0;
  persistence_scalar_event_worker_is_running = 1;
  persistence_scalar_event_worker_last_heartbeat = get_time();
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

int persistence_scalar_event_worker_stop(int drain_remaining)
{
  int was_running;
  int stuck;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  was_running = persistence_scalar_event_worker_is_running ||
                persistence_scalar_event_worker_stop_pending_flag;
  stuck = was_running &&
          persistence_scalar_event_worker_last_heartbeat != 0 &&
          (int)(get_time() - persistence_scalar_event_worker_last_heartbeat) >=
            PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS;
  persistence_scalar_event_worker_stop_pending_flag = was_running ? 1 : persistence_scalar_event_worker_stop_pending_flag;
  if (stuck)
    persistence_scalar_event_worker_is_running = 0;
  persistence_scalar_event_worker_stop_requested = 1;
  persistence_scalar_event_worker_drain_requested = drain_remaining ? 1 : 0;
  pthread_cond_signal(&persistence_scalar_event_queue_cond);
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  if (was_running)
  {
    if (!persistence_worker_timed_join(persistence_scalar_event_worker_thread))
    {
      logit("logs/log/debug",
            "PERSISTENCE: domain=scalar_event owner=worker action=stop_timeout detail=worker stop did not complete within %d sec; keeping stop gate set",
            PERSISTENCE_WORKER_STOP_JOIN_TIMEOUT_SECS);
      return 0;
    }

    pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
    persistence_scalar_event_worker_stop_pending_flag = 0;
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
  }
  return 1;
}

int persistence_scalar_event_worker_running(void)
{
  int running;
  int kill_rc;
  time_t last;
  int age;

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
     * A thread that is still alive but has not advanced its heartbeat
     * for too long is also treated as unavailable so the main thread can
     * fail closed and fall back to synchronous persistence instead of
     * waiting forever on a wedged worker.
     */
    if (persistence_scalar_event_worker_stop_pending_flag)
    {
      running = 0;
    }
    else if (persistence_scalar_event_worker_in_write)
    {
      running = 1;
    }
    else
    {
      kill_rc = pthread_kill(persistence_scalar_event_worker_thread, 0);
      if (kill_rc == ESRCH)
      {
        /* ESRCH is not a safe reap proof. Quarantine until join. */
        persistence_scalar_event_worker_stop_pending_flag = 1;
        running = 0;
      }
      else
      {
        last = persistence_scalar_event_worker_last_heartbeat;
        age = last ? (int)(get_time() - last) : -1;
        if (age >= PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS)
        {
          persistence_scalar_event_worker_stop_pending_flag = 1;
          running = 0;
        }
      }
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
  unsigned long long persistence_large_event_worker_generation = 0;
  int should_write;
  int write_ok;

  (void) unused;

  if (!persistence_worker_mysql_thread_init("large persistence worker"))
  {
    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    persistence_large_event_worker_stop_pending_flag = 1;
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
    return NULL;
  }

  while (1)
  {
    should_write = 0;

    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    /* Deadlock-detection heartbeat: see item worker. */
    persistence_large_event_worker_last_heartbeat = get_time();
    while (persistence_large_event_queue.count <= 0 &&
           !persistence_large_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_large_event_queue_cond,
                        &persistence_large_event_queue_mutex);
      persistence_large_event_worker_last_heartbeat = get_time();
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
      persistence_large_event_worker_generation =
          persistence_large_event_queue.generations[persistence_large_event_queue.head];
      should_write = 1;
    }
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);

    if (!should_write)
      continue;

    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    persistence_large_event_worker_in_write = 1;
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);

    write_ok = persistence_large_event_worker_writer ?
      persistence_large_event_worker_writer(line,
                                             persistence_large_event_worker_context) : 1;

    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    persistence_large_event_worker_in_write = 0;
    if (write_ok)
    {
      if (persistence_large_event_queue.count > 0 &&
          persistence_large_event_queue.generations[
              persistence_large_event_queue.head] ==
              persistence_large_event_worker_generation)
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

  persistence_worker_mysql_thread_end();

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

  if (persistence_large_event_worker_stop_pending_flag)
  {
    /* Quarantine is cleared only by the stop path after a successful join. */
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
    return 0;
  }

  persistence_large_event_worker_writer = writer;
  persistence_large_event_worker_context = context;
  persistence_large_event_worker_stop_requested = 0;
  persistence_large_event_worker_drain_requested = 0;
  persistence_large_event_worker_is_running = 1;
  persistence_large_event_worker_last_heartbeat = get_time();
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

int persistence_large_event_worker_stop(int drain_remaining)
{
  int was_running;
  int stuck;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  was_running = persistence_large_event_worker_is_running ||
                persistence_large_event_worker_stop_pending_flag;
  stuck = was_running &&
          persistence_large_event_worker_last_heartbeat != 0 &&
          (int)(get_time() - persistence_large_event_worker_last_heartbeat) >=
            PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS;
  persistence_large_event_worker_stop_pending_flag = was_running ? 1 : persistence_large_event_worker_stop_pending_flag;
  if (stuck)
    persistence_large_event_worker_is_running = 0;
  persistence_large_event_worker_stop_requested = 1;
  persistence_large_event_worker_drain_requested = drain_remaining ? 1 : 0;
  pthread_cond_signal(&persistence_large_event_queue_cond);
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  if (was_running)
  {
    if (!persistence_worker_timed_join(persistence_large_event_worker_thread))
    {
      logit("logs/log/debug",
            "PERSISTENCE: domain=large_event owner=worker action=stop_timeout detail=worker stop did not complete within %d sec; keeping stop gate set",
            PERSISTENCE_WORKER_STOP_JOIN_TIMEOUT_SECS);
      return 0;
    }

    pthread_mutex_lock(&persistence_large_event_queue_mutex);
    persistence_large_event_worker_stop_pending_flag = 0;
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
  }
  return 1;
}

int persistence_large_event_worker_running(void)
{
  int running;
  int kill_rc;
  time_t last;
  int age;

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
     * A thread that is still alive but has not advanced its heartbeat
     * for too long is also treated as unavailable so the main thread can
     * fail closed and fall back to synchronous persistence instead of
     * waiting forever on a wedged worker.
     */
    if (persistence_large_event_worker_stop_pending_flag)
    {
      running = 0;
    }
    else if (persistence_large_event_worker_in_write)
    {
      running = 1;
    }
    else
    {
      kill_rc = pthread_kill(persistence_large_event_worker_thread, 0);
      if (kill_rc == ESRCH)
      {
        /* ESRCH is not a safe reap proof. Quarantine until join. */
        persistence_large_event_worker_stop_pending_flag = 1;
        running = 0;
      }
      else
      {
        last = persistence_large_event_worker_last_heartbeat;
        age = last ? (int)(get_time() - last) : -1;
        if (age >= PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS)
        {
          persistence_large_event_worker_stop_pending_flag = 1;
          running = 0;
        }
      }
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

  age = (int)(get_time() - last);
  return age >= 0 ? age : 0;
}

int persistence_item_event_worker_stuck(int threshold_secs)
{
  time_t last;
  int age;

  if (threshold_secs <= 0)
    threshold_secs = PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  if (!persistence_item_event_worker_is_running ||
      persistence_item_event_worker_stop_pending_flag ||
      persistence_item_event_worker_in_write)
  {
    pthread_mutex_unlock(&persistence_item_event_queue_mutex);
    return 0;
  }
  last = persistence_item_event_worker_last_heartbeat;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  if (last == 0)
    return 0;

  age = (int)(get_time() - last);
  if (age < 0)
    age = 0;
  return age >= threshold_secs;
}

int persistence_item_event_worker_stop_pending(void)
{
  int stop_in_progress;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  stop_in_progress = persistence_item_event_worker_stop_pending_flag;
  pthread_mutex_unlock(&persistence_item_event_queue_mutex);

  return stop_in_progress;
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

  age = (int)(get_time() - last);
  return age >= 0 ? age : 0;
}

int persistence_scalar_event_worker_stuck(int threshold_secs)
{
  time_t last;
  int age;

  if (threshold_secs <= 0)
    threshold_secs = PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  if (!persistence_scalar_event_worker_is_running ||
      persistence_scalar_event_worker_stop_pending_flag ||
      persistence_scalar_event_worker_in_write)
  {
    pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);
    return 0;
  }
  last = persistence_scalar_event_worker_last_heartbeat;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  if (last == 0)
    return 0;

  age = (int)(get_time() - last);
  if (age < 0)
    age = 0;
  return age >= threshold_secs;
}

int persistence_scalar_event_worker_stop_pending(void)
{
  int stop_in_progress;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  stop_in_progress = persistence_scalar_event_worker_stop_pending_flag;
  pthread_mutex_unlock(&persistence_scalar_event_queue_mutex);

  return stop_in_progress;
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

  age = (int)(get_time() - last);
  return age >= 0 ? age : 0;
}

int persistence_large_event_worker_stuck(int threshold_secs)
{
  time_t last;
  int age;

  if (threshold_secs <= 0)
    threshold_secs = PERSISTENCE_WORKER_HEARTBEAT_STUCK_SECS;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  if (!persistence_large_event_worker_is_running ||
      persistence_large_event_worker_stop_pending_flag ||
      persistence_large_event_worker_in_write)
  {
    pthread_mutex_unlock(&persistence_large_event_queue_mutex);
    return 0;
  }
  last = persistence_large_event_worker_last_heartbeat;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  if (last == 0)
    return 0;

  age = (int)(get_time() - last);
  if (age < 0)
    age = 0;
  return age >= threshold_secs;
}

int persistence_large_event_worker_stop_pending(void)
{
  int stop_in_progress;

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  stop_in_progress = persistence_large_event_worker_stop_pending_flag;
  pthread_mutex_unlock(&persistence_large_event_queue_mutex);

  return stop_in_progress;
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

/* =================================================================
 * persistence_sql_escape_field — SQL string escaping for safe
 * embedding of player/item names in SQL queries.
 * Doubles apostrophes and backslashes; replaces pipe, CR, LF with space.
 * Returns "none" for NULL input; returns "" for NULL/bad buffer.
 * ================================================================= */
const char *persistence_sql_escape_field(const char *in, char *buf,
                                         int buf_size)
{
  int i, j;

  if (!buf || buf_size <= 0)
    return "";

  if (!in)
  {
    snprintf(buf, buf_size, "none");
    return buf;
  }

  for (i = 0, j = 0; in[i] && j < buf_size - 1; i++)
  {
    if (in[i] == '\'' || in[i] == '\\')
    {
      if (j + 1 >= buf_size - 1)
        break;
      buf[j++] = in[i];
      buf[j++] = in[i];
    }
    else if (in[i] == '|' || in[i] == '\r' || in[i] == '\n')
    {
      buf[j++] = ' ';
    }
    else
    {
      buf[j++] = in[i];
    }
  }
  buf[j] = '\0';
  return buf;
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
