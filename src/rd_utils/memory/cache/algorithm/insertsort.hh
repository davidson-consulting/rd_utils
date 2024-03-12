#pragma once


#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>
#include "common.hh"


namespace rd_utils::memory::cache::algorithm {

  template <typename T>
  void insert_sort (T * buffer, collection::CacheArray<T> & arr, uint32_t low, uint32_t n) {
    int64_t i, j;

    for (i = low  ; i < (n + low) ; i++) {
      T key = arr.get (i);
      j = i - 1;

      while (j >= 0 && arr.get (j) > key) {
        arr.set (j + 1, arr.get (j));
        j = j - 1;
      }

      arr.set (j + 1, key);
    }
  }

  template <typename T>
  void insert_sort (T * array, uint32_t low, uint32_t end) {
    int64_t i, j;
    
    for (i = low ; i < end ; i++) {
      T key = array [i];
      j = i - 1;
      while (j >= 0 && array [j] > key) {
        array [j + 1] = array [j];
        j -= 1;
      }

      array [j + 1] = key;
    }

  }


}
