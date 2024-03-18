#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename Z, typename T, typename F>
  collection::CacheArray<T> map (collection::CacheArray<T> && array, F func) {
    uint32_t len = array.len ();
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      array.map (buffer, ARRAY_BUFFER_SIZE, func);
      free (buffer);
    }

    return std::move (array);
  }

}
