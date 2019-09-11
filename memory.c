/* -*- C -*- */

#ifndef KODI_C_LIBRARY_Memory
#define KODI_C_LIBRARY_Memory

#include <assert.h>

long unsigned int kodi_memory_malloced = 0;
  // Incremented each time we allocate memory; decremented
  // each time we free it.

void * kmalloc(size_t s)
/* A substitute for malloc. */
 {++kodi_memory_malloced;
  void *p = malloc(s);
  if (p == NULL)
     {printf("I failed to allocate %lu bytes of memory.\n", s);
      exit(1);}
  return p;}

void kfree(void *p)
/* A substitute for free. */
 {--kodi_memory_malloced;
  free(p);}

void assert_mem(void)
/* Ensures that all the memory that was allocated was freed.
Call this just before your program exits. */
 {assert(kodi_memory_malloced == 0);}

#endif
