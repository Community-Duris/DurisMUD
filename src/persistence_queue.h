#ifndef __PERSISTENCE_QUEUE_H_INCLUDED__
#define __PERSISTENCE_QUEUE_H_INCLUDED__

#define PERSISTENCE_EVENT_QUEUE_CAPACITY 4096
#define PERSISTENCE_EVENT_MAX_LEN 1024

int persistence_item_event_queue_enqueue(const char *line);
int persistence_item_event_queue_dequeue(char *out, int out_size);
int persistence_item_event_queue_pending(void);
unsigned long persistence_item_event_queue_dropped(void);
void persistence_item_event_queue_clear_dropped(void);
void persistence_item_event_queue_reset(void);

#endif
