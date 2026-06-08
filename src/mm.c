#include "prototypes.h"
#include "structs.h"
#include "utils.h"
#include "mm.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "defines.h"

#ifdef _OSX_
#define MAP_ANONYMOUS MAP_ANON
#endif

static inline void *__constant_c_memset(void *, unsigned long, size_t);

struct mm_ds_list *mmds_list = NULL;

// This code is kind of deprecated.  CRT should be at least as good as this
static inline void *__constant_c_memset(void *s, unsigned long c, size_t count)
{
	__asm__ __volatile__("cld\n\t"
	                     "rep ; stosl\n\t"
	                     "testb $2,%b1\n\t"
	                     "je 1f\n\t"
	                     "stosw\n"
	                     "1:\ttestb $1,%b1\n\t"
	                     "je 2f\n\t"
	                     "stosb\n"
	                     "2:"
	                     : /* no output registers */
	                     : "a"(c), "q"(count), "c"(count / 4), "D"((long)s)
	                     : "cx", "di", "memory");
	return (s);
}

struct mm_ds *mm_create(const char *name, size_t size, size_t next_off, unsigned pages)
{
	struct mm_ds      *mmds;
	struct mm_ds_list *list_entry;

	CREATE(mmds, mm_ds, 1, MEM_TAG_MEMMAN);
	strlcpy(mmds->name, name, sizeof mmds->name);

	mmds->head = mmds->tail = NULL;
	mmds->size              = size;
	mmds->next_off          = next_off;
	mmds->chunk_size        = pages;
#ifdef MM_STATS
	mmds->pages_owned = mmds->objs_used = mmds->bytes_wasted = 0;
#endif

	CREATE(list_entry, mm_ds_list, 1, MEM_TAG_MMLIST);

	list_entry->mmds = mmds;
	list_entry->next = mmds_list;
	mmds_list        = list_entry;

	return mmds;
}

void mm_release(struct mm_ds *mmds, void *mem)
{
	*((char **)((char *)mem + mmds->next_off)) = NULL;

	// Then put it at the end of the list.
	if (mmds->tail)
	{
		*((char **)(mmds->tail + mmds->next_off)) = (char *)mem;
		mmds->tail                                = (char *)mem;
	}
	else
	{
		mmds->head = mmds->tail = (char *)mem;
	}

#ifdef MM_STATS
	mmds->objs_used--;
#endif
}

void *_mm_get(struct mm_ds *mmds, const char *file, int line)
{
	char *mem;

	// If we're outta memory, get more.
	if (!mmds->head)
	{
		mm_alloc_chunk(mmds);
	}

	mem        = mmds->head;
	mmds->head = *((char **)(mem + mmds->next_off));
	if (!mmds->head)
		mmds->tail = NULL;

#ifdef MM_STATS
	mmds->objs_used++;
#endif
	memset(mem, 0, mmds->size);

	// if(!strcmp(mmds->name, "PC_ONLY"))
	//	debug("mm_get: pool: %s, file: %s, line: %d", mmds->name, file, line);

	return mem;
}

void mm_alloc_chunk(struct mm_ds *mmds)
{
	char  *more;
	size_t offset = 0;
	size_t howmuch;

	howmuch = mmds->chunk_size * 4096;

	/*
	   allocate the memory
	 */
	//     CREATE(more, char, howmuch);

	more = (char *)mmap(0, howmuch, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (more == (char *)-1)
	{
		int tmp_errno = errno;

		fatal_boot_error("mm", "mmap failed for pool '%s' (%zu bytes): %s", mmds->name, howmuch,
		                 strerror(tmp_errno));
	}
	/*
	   should I memset it to all 0's?
	 */

#ifdef MM_STATS
	mmds->pages_owned += mmds->chunk_size;
	mmds->bytes_wasted += (howmuch % mmds->size);
#endif

	/*
	   split the memory and stuff it into the list
	 */

	/*
	   note that we want to stuff it at the HEAD of the list.  Doing this will
	   allow the newly allocated memory to be used first
	 */

	/*
	   if the queue is empty, initialize it
	 */

	if (!mmds->head)
	{
		mmds->head = mmds->tail = more;
		offset += mmds->size;
	}
	/*
	   now, for every 'mmds->size' bytes, add a pointer to the queue.
	 */

	while ((offset + mmds->size) <= howmuch)
	{
		*((char **)(more + offset + mmds->next_off)) = mmds->head;
		mmds->head                                   = (char *)(more + offset);
		offset += mmds->size;
	}
}

/*
   okay... this function is used to decide what the best "chunk size"
   would be for a given structure.  The best size is defined as having
   the least waste.

   size = sizeof the structure
   min  = least number of structures available in each chunk
   max  = max number of structures available in each chunk

   The min and max values are used to provide a "range" in which you
   might want to allocate at one time.  For a structure which you'll
   need 50k instances of, the "min" value is likely to be quite high
   in order to prevent having to constantly map new chunks.
   On the other hand, if you only need a dozen instances of a small
   struct, the max value will be low, so as to reduce the number of
   unused objects.

   This function will return the "best" chunk size (passed to
   mm_create) given the size, min, and max.

 */

unsigned mm_find_best_chunk(int size, int min, int max)
{
	int i, j = 1, waste, best = size;

	for (i = min; i <= max; i++)
	{
		waste = (((size * i) / 4096 + 1) * 4096) % size;
		if (waste < best)
		{
			best = waste;
			j    = i;
		}
	}
	return ((size * j) / 4096 + 1);
}
