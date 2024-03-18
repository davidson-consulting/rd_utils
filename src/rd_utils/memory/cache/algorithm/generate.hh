#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>
#include <tuple>
#include <functional>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename Z, typename F>
  collection::CacheArray<Z> generate (uint32_t len, F func) {
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      result.generate (buffer, ARRAY_BUFFER_SIZE, func);

      free (buffer);
    }

    return result;
  }



}
