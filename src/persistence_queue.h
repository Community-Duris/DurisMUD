#ifndef __PERSISTENCE_QUEUE_H_INCLUDED__
#define __PERSISTENCE_QUEUE_H_INCLUDED__

#define PERSISTENCE_EVENT_QUEUE_CAPACITY 4096
#define PERSISTENCE_EVENT_MAX_LEN 1024
#define PERSISTENCE_WORKER_RETRY_USEC 100000

typedef int (*persistence_item_event_writer)(const char *line, void *context);

int persistence_item_event_queue_enqueue(const char *line);
int persistence_item_event_queue_dequeue(char *out, int out_size);
int persistence_item_event_queue_pending(void);
unsigned long persistence_item_event_queue_dropped(void);
void persistence_item_event_queue_clear_dropped(void);
void persistence_item_event_queue_reset(void);

int persistence_item_event_worker_start(persistence_item_event_writer writer,
                                        void *context);
void persistence_item_event_worker_stop(int drain_remaining);
int persistence_item_event_worker_running(void);
unsigned long persistence_item_event_worker_written(void);
unsigned long persistence_item_event_worker_write_failures(void);

#endif
