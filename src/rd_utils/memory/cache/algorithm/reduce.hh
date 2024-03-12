#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename T, typename F>
  T reduce (collection::CacheArray<T> & array, F func) {
    T * buffer = reinterpret_cast <T*> (malloc (ARRAY_BUFFER_SIZE * sizeof (T)));
    auto p = array.puller (0, buffer, ARRAY_BUFFER_SIZE);
    p.next ();
    T current = p.current ();
    while (p.next ()) {
      current = func (current, p.current ());
    }

    free (buffer);
    return current;
  }


  template <typename T, typename F>
  T reduce (const std::vector <T> & array, F func) {
    T current = array [0];
    uint32_t i = 1;
    while (i < array.size ()) {
      current = func (current, array [i]);
      i ++;
    }
    return current;
  }



}
