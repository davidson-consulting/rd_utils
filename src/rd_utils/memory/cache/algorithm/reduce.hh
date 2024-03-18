#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename T, typename F>
  T reduce (collection::CacheArray<T> & array, F func) {
    if (array.len () == 0) throw std::runtime_error ("Cannot reduce empty array");
    T * buffer = reinterpret_cast <T*> (malloc (ARRAY_BUFFER_SIZE * sizeof (T)));
    auto res = array.reduce (buffer, ARRAY_BUFFER_SIZE, func);
    free (buffer);

    return res;
  }

  template <typename T, typename F>
  T reduce (const std::vector <T> & array, F func) {
    T current = array [0];
    for (size_t i = 1 ; i < array.size () ; i++) {
      current = func (current, array [i]);
    }
    return current;
  }



}
