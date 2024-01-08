#include "gc.hh"
#include <gc/gc.h>

const garbage_collection_t GC;

void * operator new (size_t cbSize, const garbage_collection_t &) {
  char * mem = (char*) GC_malloc (cbSize);

  // Touching the blocks to actually perform the allocation
  for (size_t i = 0; i < cbSize; i += 4096) mem[i] = 0;
  mem[cbSize - 1] = 0;

  return (void*) mem;
}
