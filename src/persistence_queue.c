#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "persistence_queue.h"

struct persistence_event_queue_data
{
  char events[PERSISTENCE_EVENT_QUEUE_CAPACITY][PERSISTENCE_EVENT_MAX_LEN];
  int head;
  int tail;
  int count;
  unsigned long dropped;
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

static void persistence_item_event_queue_pop_head(void)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;

  if (q->count <= 0)
    return;

  q->head = (q->head + 1) % PERSISTENCE_EVENT_QUEUE_CAPACITY;
  q->count--;
}

int persistence_item_event_queue_enqueue(const char *line)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;
  int ok = 1;

  if (!line || !*line)
    return 0;

  pthread_mutex_lock(&persistence_item_event_queue_mutex);

  if (q->count >= PERSISTENCE_EVENT_QUEUE_CAPACITY)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], PERSISTENCE_EVENT_MAX_LEN, "%s", line);
    q->tail = (q->tail + 1) % PERSISTENCE_EVENT_QUEUE_CAPACITY;
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
  memset(&persistence_item_event_queue, 0, sizeof(persistence_item_event_queue));
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
    while (persistence_item_event_queue.count <= 0 &&
           !persistence_item_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_item_event_queue_cond,
                        &persistence_item_event_queue_mutex);
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

  pthread_mutex_lock(&persistence_item_event_queue_mutex);
  running = persistence_item_event_worker_is_running;
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

  q->head = (q->head + 1) % PERSISTENCE_EVENT_QUEUE_CAPACITY;
  q->count--;
}

static void persistence_large_event_queue_pop_head(void)
{
  persistence_event_queue_data *q = &persistence_large_event_queue;

  if (q->count <= 0)
    return;

  q->head = (q->head + 1) % PERSISTENCE_LARGE_EVENT_QUEUE_CAPACITY;
  q->count--;
}

int persistence_scalar_event_queue_enqueue(const char *line)
{
  persistence_event_queue_data *q = &persistence_scalar_event_queue;
  int ok = 1;

  if (!line || !*line)
    return 0;

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);

  if (q->count >= PERSISTENCE_EVENT_QUEUE_CAPACITY)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], PERSISTENCE_EVENT_MAX_LEN, "%s", line);
    q->tail = (q->tail + 1) % PERSISTENCE_EVENT_QUEUE_CAPACITY;
    q->count++;
    pthread_cond_signal(&persistence_scalar_event_queue_cond);
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

  if (q->count >= PERSISTENCE_LARGE_EVENT_QUEUE_CAPACITY)
  {
    q->dropped++;
    ok = 0;
  }
  else
  {
    snprintf(q->events[q->tail], PERSISTENCE_LARGE_EVENT_MAX_LEN, "%s", line);
    q->tail = (q->tail + 1) % PERSISTENCE_LARGE_EVENT_QUEUE_CAPACITY;
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
  memset(&persistence_large_event_queue, 0, sizeof(persistence_large_event_queue));
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
  memset(&persistence_scalar_event_queue, 0, sizeof(persistence_scalar_event_queue));
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
    while (persistence_scalar_event_queue.count <= 0 &&
           !persistence_scalar_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_scalar_event_queue_cond,
                        &persistence_scalar_event_queue_mutex);
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

  pthread_mutex_lock(&persistence_scalar_event_queue_mutex);
  running = persistence_scalar_event_worker_is_running;
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
    while (persistence_large_event_queue.count <= 0 &&
           !persistence_large_event_worker_stop_requested)
    {
      pthread_cond_wait(&persistence_large_event_queue_cond,
                        &persistence_large_event_queue_mutex);
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

  pthread_mutex_lock(&persistence_large_event_queue_mutex);
  running = persistence_large_event_worker_is_running;
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
