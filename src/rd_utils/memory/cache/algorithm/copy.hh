#pragma once



#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename T>
  collection::CacheArray<T> copy (collection::CacheArray<T> & array) {
    collection::CacheArray<T> result (array.len ());

    if (array.len () == 0) return result;
    result.copyRaw (array);


    return result;
  }

}
