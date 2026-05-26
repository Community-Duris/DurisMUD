#include <stdio.h>
#include <string.h>

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

int persistence_item_event_queue_enqueue(const char *line)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;

  if (!line || !*line)
    return 0;

  if (q->count >= PERSISTENCE_EVENT_QUEUE_CAPACITY)
  {
    q->dropped++;
    return 0;
  }

  snprintf(q->events[q->tail], PERSISTENCE_EVENT_MAX_LEN, "%s", line);
  q->tail = (q->tail + 1) % PERSISTENCE_EVENT_QUEUE_CAPACITY;
  q->count++;

  return 1;
}

int persistence_item_event_queue_dequeue(char *out, int out_size)
{
  persistence_event_queue_data *q = &persistence_item_event_queue;

  if (!out || out_size <= 0 || q->count <= 0)
    return 0;

  snprintf(out, out_size, "%s", q->events[q->head]);
  q->head = (q->head + 1) % PERSISTENCE_EVENT_QUEUE_CAPACITY;
  q->count--;

  return 1;
}

int persistence_item_event_queue_pending(void)
{
  return persistence_item_event_queue.count;
}

unsigned long persistence_item_event_queue_dropped(void)
{
  return persistence_item_event_queue.dropped;
}

void persistence_item_event_queue_clear_dropped(void)
{
  persistence_item_event_queue.dropped = 0;
}

void persistence_item_event_queue_reset(void)
{
  memset(&persistence_item_event_queue, 0, sizeof(persistence_item_event_queue));
}
