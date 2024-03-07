#pragma once


#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

namespace rd_utils::memory::cache::algorithm {

#define BUFFER_SIZE 1024

  uint32_t flp2 (uint32_t x)
  {
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x - (x >> 1);
  }

  template <typename T>
  void compAndSwap (T * a, T * b, uint32_t i, uint32_t dir) {
    auto atI = a [i];
    auto atJ = b [i];

    if (dir == (atI > atJ)) {
      a [i] = atJ;
      b [i] = atI;
    }
  }

  template <typename T>
  void bitonicMergeBlock (T * a, uint32_t low, uint32_t nb, uint32_t dir) {
    if (nb > 1) {
      int k = flp2 (nb - 1);
      for (int i = low ; i < low + nb - k ; i++) {
        auto atI = a [i];
        auto atJ = a [i + k];

        if (dir == (atI > atJ)) {
          a [i] = atJ;
          a [i + k] = atI;
        }
      }

      bitonicMergeBlock (a, low, k, dir);
      bitonicMergeBlock (a, low + k, nb - k, dir);
    }
  }

  template <typename T>
  void bitonicMerge (T * buffer, uint32_t BLK_SIZE, collection::CacheArray<T> & arr, uint32_t low, uint32_t nb, uint32_t dir) {
    if (nb > 1) {
      uint32_t k = flp2 (nb - 1);
      uint32_t end = low + nb - k;
      auto locBLK_SIZE = BLK_SIZE / 2;
      for (uint32_t i = low ; i < end ; i += locBLK_SIZE) {
        auto nb_read = end - i < locBLK_SIZE ? end - i : locBLK_SIZE;

        arr.getNb (i, buffer, nb_read);
        arr.getNb (i + k, buffer + nb_read, nb_read);

        for (uint32_t z = 0 ; z < nb_read ; z++) {
          compAndSwap (buffer, buffer + nb_read, z, dir);
        }

        arr.setNb (i, buffer, nb_read);
        arr.setNb (i + k, buffer + nb_read, nb_read);
      }

      if (k <= BLK_SIZE) {
        arr.getNb (low, buffer, k);
        bitonicMergeBlock (buffer, 0, k, dir);
        arr.setNb (low, buffer, k);
      } else {
        bitonicMerge (buffer, BLK_SIZE, arr, low, k, dir);
      }

      if (nb - k <= BLK_SIZE) {
        arr.getNb (low + k, buffer, nb - k);
        bitonicMergeBlock (buffer, 0, nb - k, dir);
        arr.setNb (low + k, buffer, nb - k);
      } else {
        bitonicMerge (buffer, BLK_SIZE, arr, low + k, nb - k, dir);
      }
    }
  }


  template <typename T>
  void bitonicSort (T * buffer, uint32_t BLK_SIZE, collection::CacheArray<T> & arr, uint32_t low, uint32_t nb, uint32_t dir) {
    if (nb > 1) {
      auto k = nb / 2;
      bitonicSort (buffer, BLK_SIZE, arr, low, k, !dir);
      bitonicSort (buffer, BLK_SIZE, arr, low + k, nb - k, dir);

      if (nb <= BLK_SIZE) {
        arr.getNb (low, buffer, nb);
        bitonicMergeBlock (buffer, 0, nb, dir);
        arr.setNb (low, buffer, nb);
      } else {
        bitonicMerge (buffer, BLK_SIZE, arr, low, nb, dir);
      }
    }
  }

  template <typename T>
  void bitonicSort (collection::CacheArray<T> & arr) {
    T * buffer = reinterpret_cast <T*> (malloc (BUFFER_SIZE * sizeof (T)));
    bitonicSort (buffer, BUFFER_SIZE, arr, 0, arr.len (), 1);
    free (buffer);
  }

}
