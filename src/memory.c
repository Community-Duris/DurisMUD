/* ************************************************************************
*  File: memory.c                                Part of Duris		  *
*  Usage: functions for debugging memory                                  *
************************************************************************ */

/**************************************************************************
* Using the memory debugging                                              *
*                                                                         *
* The memory debugging option keeps a list of all memory allocated with   *
* the CREATE or RECREATE macros, and which file and line it was allocated *
* at.  FREE removes the entry from the list.  When the game is shut down, *
* a list of all memory not FREEd, which file and line it was created, and *
* how many bytes will be recorded to log/memory.log.  It will also log    *
* any attempts to FREE an object not in its list of allocated memory,     *
* for example, memory already FREEd or memory not alloced with CREATE or  *
* RECREATE.                                                               *
* Optionally, all allocations and frees can be logged.                    *
*                                                                         *
* To turn on this feature, define MEMCHK and recompile.  Defining         *
* MEMCHK = 2 will turn on the additional logging of the allocs and frees  *
**************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "structs.h"
#include "utils.h"
#include "prototypes.h"

#define MEMORY_LOG    "logs/log/memory"

mem_usage mem_used[52];

#define MEM_USAGE_INITCASE(i, t) \
  case i: \
    mem_used[i].tag = t; \
    break;

bool muinit = false;

void init_mem_used()
{
  memset(mem_used, 0, sizeof(mem_used));

  for(int i = 0; i < 52; i++)
  {
    switch(i)
    {
      MEM_USAGE_INITCASE(0, MEM_TAG_STRING);
      MEM_USAGE_INITCASE(1, MEM_TAG_BUFFER  )
      MEM_USAGE_INITCASE(2, MEM_TAG_ARRAY   )
      MEM_USAGE_INITCASE(3, MEM_TAG_SOCMSG  )
      MEM_USAGE_INITCASE(4, MEM_TAG_SNOOP   )
      MEM_USAGE_INITCASE(5, MEM_TAG_WIZBAN  )
      MEM_USAGE_INITCASE(6, MEM_TAG_BAN     )
      MEM_USAGE_INITCASE(7, MEM_TAG_DIRDATA )
      MEM_USAGE_INITCASE(8, MEM_TAG_TXTBLK  )
      MEM_USAGE_INITCASE(9, MEM_TAG_REGNODE )
      MEM_USAGE_INITCASE(10, MEM_TAG_TBLDATA )
      MEM_USAGE_INITCASE(11, MEM_TAG_TBLELEM )
      MEM_USAGE_INITCASE(12, MEM_TAG_IDXDATA )
      MEM_USAGE_INITCASE(13, MEM_TAG_ROOMDAT )
      MEM_USAGE_INITCASE(14, MEM_TAG_ZONEDAT )
      MEM_USAGE_INITCASE(15, MEM_TAG_SECTDAT )
      MEM_USAGE_INITCASE(16, MEM_TAG_RESET   )
      MEM_USAGE_INITCASE(17, MEM_TAG_NPCONLY )
      MEM_USAGE_INITCASE(18, MEM_TAG_EXDESCD )
      MEM_USAGE_INITCASE(19, MEM_TAG_EDITDAT )
      MEM_USAGE_INITCASE(20, MEM_TAG_SHPCHNG )
      MEM_USAGE_INITCASE(21, MEM_TAG_SHIPREG )
      MEM_USAGE_INITCASE(22, MEM_TAG_SACKREC )
      MEM_USAGE_INITCASE(23, MEM_TAG_HUNTDAT )
      MEM_USAGE_INITCASE(24, MEM_TAG_MEMMAN  )
      MEM_USAGE_INITCASE(25, MEM_TAG_MMLIST  )
      MEM_USAGE_INITCASE(26, MEM_TAG_REMEMBD )
      MEM_USAGE_INITCASE(27, MEM_TAG_MOBMEM  )
      MEM_USAGE_INITCASE(28, MEM_TAG_OLCDATA )
      MEM_USAGE_INITCASE(29, MEM_TAG_QSTMSG  )
      MEM_USAGE_INITCASE(30, MEM_TAG_QSTCOMP )
      MEM_USAGE_INITCASE(31, MEM_TAG_QSTGOAL )
      MEM_USAGE_INITCASE(32, MEM_TAG_SHOPDAT )
      MEM_USAGE_INITCASE(33, MEM_TAG_SHOPBUY )
      MEM_USAGE_INITCASE(34, MEM_TAG_FOLLOW  )
      MEM_USAGE_INITCASE(35, MEM_TAG_SHIPAI  )
      MEM_USAGE_INITCASE(36, MEM_TAG_SHIPGRP )
      MEM_USAGE_INITCASE(37, MEM_TAG_SHIPDAT )
      MEM_USAGE_INITCASE(38, MEM_TAG_POSLIST )
      MEM_USAGE_INITCASE(39, MEM_TAG_ZSTREAM )
      MEM_USAGE_INITCASE(40, MEM_TAG_EVTBUF  )
      MEM_USAGE_INITCASE(41, MEM_TAG_NQACTOR )
      MEM_USAGE_INITCASE(42, MEM_TAG_NQSTR   )
      MEM_USAGE_INITCASE(43, MEM_TAG_NQINST  )
      MEM_USAGE_INITCASE(44, MEM_TAG_NQITEM  )
      MEM_USAGE_INITCASE(45, MEM_TAG_NQSKILL )
      MEM_USAGE_INITCASE(46, MEM_TAG_NQRWRD  )
      MEM_USAGE_INITCASE(47, MEM_TAG_NQACTN  )
      MEM_USAGE_INITCASE(48, MEM_TAG_NQATMP  )
      MEM_USAGE_INITCASE(49, MEM_TAG_NQQST   )
      MEM_USAGE_INITCASE(50, MEM_TAG_OTHER   )
      MEM_USAGE_INITCASE(51, "");
    }
  }
  muinit = true;
}

void increment_mem_used(char* tag, size_t size)
{
  for(int i = 0; i < 52; i++)
  {
    if(strcmp(mem_used[i].tag, tag))
      continue;
    mem_used[i].allocs++;
    mem_used[i].size += size;
    return;
  }
}

void decrement_mem_used(char* tag, size_t size)
{
  for(int i = 0; i < 52; i++)
  {
    if(strcmp(mem_used[i].tag, tag))
      continue;
    mem_used[i].allocs--;
    mem_used[i].size -= size;
    return;
  }
}

FILE    *mem_log = NULL;
ALLOCATION_HEADER *allocation_list = NULL;    /* The list of memory alloced so far */
long allocation_list_node_count = 0;

/* adds a piece of memory to the list */
void* getmem(size_t size, char *tag, char *file, int line)
{
  ALLOCATION_HEADER *NewAllocation;

  if(!muinit)
    init_mem_used();

#if MEMCHK > 1
  /* open the memory log if not already */
  if (!mem_log)
  {
    if ((mem_log = fopen(MEMORY_LOG, "w")) == NULL)
    {
      logit(LOG_EXIT, "fopen memory log");
      raise(SIGSEGV);
    }
  }


  fprintf(mem_log, "%p: Allocating %d bytes, file %s:%d\n",
          p, size, file, line);
#endif


  if ((NewAllocation = (ALLOCATION_HEADER*) malloc(sizeof(ALLOCATION_HEADER) + size)) == NULL)
  {
    logit(LOG_EXIT, "Failed to malloc memory");
    raise(SIGSEGV);
  }

  memset(NewAllocation, 0, sizeof(ALLOCATION_HEADER) + size);
  /* init new node */
  NewAllocation->tag= tag;
  NewAllocation->size = size;
  NewAllocation->file = file;
  NewAllocation->line = line;
  NewAllocation->body = (void*) (((char*)NewAllocation) + sizeof(ALLOCATION_HEADER));

  /* add node to list */
  /*NewAllocation->next = allocation_list;
  allocation_list = NewAllocation;
  allocation_list_node_count += 1;*/

  increment_mem_used(NewAllocation->tag, NewAllocation->size);

  return NewAllocation->body;
}

void* changemem(void *p, size_t size, char *file, int line)
{
  ALLOCATION_HEADER *l, *m;

#if MEMCHK > 1
  /* open the memory log if not already */
  if (!mem_log)
  {
    if ((mem_log = fopen(MEMORY_LOG, "w")) == NULL)
    {
      logit(LOG_EXIT, "fopen memory log");
      exit(1);
    }
  }
#endif

  /* find the entry in the list */
  //for (l = NULL, m = allocation_list; m && (m->body != p); l = m, m = m->next);

  if (p)
  {
#if MEMCHK > 1
    fprintf(mem_log, "%p: realloc in file %s:%d\n", p, file, line);
#endif
    m = (ALLOCATION_HEADER*)(((char*)p) - sizeof(ALLOCATION_HEADER));
    if(m->tag[0] != 'M')
    {
      logit(LOG_EXIT, "changemem: memory failed check!");
      raise(SIGSEGV);
    }      

    //if (m == allocation_list)
    //{                           /* head of list */
    //  allocation_list = m->next;
    //}
    //else
    //{
    //  l->next = m->next;
    //}
    if((m = (ALLOCATION_HEADER*) realloc(m, sizeof(ALLOCATION_HEADER) + size)) == NULL)
    {
      logit(LOG_EXIT, "Failed to realloc memory");
      raise(SIGSEGV);
    }

    decrement_mem_used(m->tag, m->size);
    m->size = size;
    increment_mem_used(m->tag, m->size);
    m->file = file;
    m->line = line;
    m->body = (void*) (((char*)m) + sizeof(ALLOCATION_HEADER));

    //m->next = allocation_list;
    //allocation_list = m;
  }
  else
  {
#if MEMCHK > 1
    fprintf(mem_log, "Attempting to realloc non-alloced memory, file %s:%d\n", file, line);
#endif
    logit(LOG_EXIT, "RECREATE called, but memory not in allocation list!");
    raise(SIGSEGV);
  }

  return m->body;
}

/* removes a memory entry from the list */
void delmem(void *p, char *file, int line)
{
  ALLOCATION_HEADER *l, *m;

#if MEMCHK > 1
  /* open the memory log if not already */
  if (!mem_log)
  {
    if ((mem_log = fopen(MEMORY_LOG, "w")) == NULL)
    {
      logit(LOG_EXIT, "fopen memory log");
      exit(1);
    }
  }
#endif

  /* find the entry in the list */
  //for (l = NULL, m = allocation_list; m && (m->body != p); l = m, m = m->next);

  if (p)
  {
    m = (ALLOCATION_HEADER*)(((char*)p) - sizeof(ALLOCATION_HEADER));
    if(m->tag[0] != 'M')
    {
      logit(LOG_EXIT, "delmem: memory failed check!");
      raise(SIGSEGV);
    } 

#if MEMCHK > 1
    fprintf(mem_log, "%p: Freed in file %s:%d\n", p, file, line);
#endif

    //if (m == allocation_list)
    //{                           /* head of list */
    //  allocation_list = m->next;
    //}
    //else
    //{
    //  l->next = m->next;
    //}
    decrement_mem_used(m->tag, m->size);
    m->tag = (char*)0x0BADF00D;
    m->file = file;
    m->line = line;
    free(m);
    allocation_list_node_count -= 1;
  }
  else
  {
#if MEMCHK > 1
    fprintf(mem_log, "Attempting to free non-alloced memory, file %s:%d\n", file, line);
#endif
    logit(LOG_EXIT, "FREE called, but memory not in allocation list!");
    raise(SIGSEGV);
  }
}


/* writes the list of all unfreed memory to file */
void dump_mem_log(void)
{
#if MEMCHK > 1
  ALLOCATION_HEADER *l, *next;
  size_t   total = 0;

  /* open the memory log if not already */
  if (!mem_log)
  {
    if ((mem_log = fopen(MEMORY_LOG, "w")) == NULL)
    {
      logit(LOG_EXIT, "fopen memory log");
      exit(1);
    }
  }

  for (l = allocation_list; l; l = next)
  {
    next = l->next;
    total += l->size;
    fprintf(mem_log, "%d bytes with tag '%s' of unfreed memory at 0x%08x, %s:%d\n", l->size, l->tag, l, l->file, l->line);
    free((char *) l);
  }

  fprintf(mem_log, "Total unfreed memory: %d\n", total);

  fclose(mem_log);
#endif
}

void* __malloc(size_t size, char* tag, char* file, int line)
{
#ifdef MEMCHK
  return getmem(size, tag, file, line);
#else
  return malloc(size);
#endif
}

void* __realloc(void* p, size_t size, char *file, int line)
{
#ifdef MEMCHK
  return changemem(p, size, file, line);
#else
  return realloc(p, size);
#endif
}

void __free(void* p, char *file, int line)
{
#ifdef MEMCHK
  delmem(p, file, line);
#else
  free(p);
#endif
}
