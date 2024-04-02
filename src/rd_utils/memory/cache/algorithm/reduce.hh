#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename Z, typename T, typename F>
  Z reduce (collection::CacheArray<T> & array, F func, Z fst = Z ()) {
    if (array.len () == 0) return fst;

    T * buffer = reinterpret_cast <T*> (malloc (ARRAY_BUFFER_SIZE * sizeof (T)));
    auto res = array.reduce (buffer, ARRAY_BUFFER_SIZE, func, fst);
    free (buffer);

    return res;
  }

  template <typename Z, typename T, typename F>
  Z reduce (const std::vector <T> & array, F func, Z fst = Z ()) {
    Z current = fst;
    for (size_t i = 0 ; i < array.size () ; i++) {
      current = func (fst, array [i]);
    }
    return current;
  }



}
