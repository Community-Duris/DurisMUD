#include <stddef.h> /* needed for the offsetof macro */
#define MM_STATS

struct mm_ds
{
	char   name[8];
	char  *head; /* take from here */
	char  *tail; /* add to here */
	size_t size; /* try to make this a multiple of 16 */
	size_t next_off;
	int    chunk_size; /* number of pages to allocate at a time */
#ifdef MM_STATS
	size_t pages_owned;  /* how much mem have I allocated */
	size_t objs_used;    /* how many objs are "out there"? */
	size_t bytes_wasted; /* how much mem is (unusable) */
#endif
};

struct mm_ds_list
{
	struct mm_ds      *mmds;
	struct mm_ds_list *next;
};

struct mm_ds *mm_create(const char *, size_t, size_t, unsigned);
void          mm_release(struct mm_ds *, void *);
#define mm_get(mmds) _mm_get(mmds, __FILE__, __LINE__)
void    *_mm_get(struct mm_ds *, const char *, int);
void     mm_alloc_chunk(struct mm_ds *);
unsigned mm_find_best_chunk(int size, int min, int max);
