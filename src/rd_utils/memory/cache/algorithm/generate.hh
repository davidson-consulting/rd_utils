#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>
#include <tuple>
#include <functional>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename Z>
  collection::CacheArray<Z> generate (uint32_t len, Z (*func)(uint32_t)) {
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      for (uint32_t i = 0 ; i < len ; i += ARRAY_BUFFER_SIZE) {
        auto nb_write = len - i >= ARRAY_BUFFER_SIZE ? ARRAY_BUFFER_SIZE : len - i;
        for (uint32_t j = 0 ; j < nb_write ; j++) {
          buffer [j] = func (i + j);
        }

        result.setNb (i, buffer, nb_write);
      }

      free (buffer);
    }

    return result;
  }

  template <typename Z, typename X>
  collection::CacheArray<Z> generate (uint32_t len, X * x, Z (X::*func)(uint32_t)) {
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      for (uint32_t i = 0 ; i < len ; i += ARRAY_BUFFER_SIZE) {
        auto nb_write = len - i >= ARRAY_BUFFER_SIZE ? ARRAY_BUFFER_SIZE : len - i;
        for (uint32_t j = 0 ; j < nb_write ; j++) {
          buffer [j] = (x->* (func)) (i + j);
        }

        result.setNb (i, buffer, nb_write);
      }

      free (buffer);
    }

    return result;
  }

  template <typename Z, typename F>
  collection::CacheArray<Z> generate (uint32_t len, F func) {
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      for (uint32_t i = 0 ; i < len ; i += ARRAY_BUFFER_SIZE) {
        auto nb_write = len - i >= ARRAY_BUFFER_SIZE ? ARRAY_BUFFER_SIZE : len - i;
        for (uint32_t j = 0 ; j < nb_write ; j++) {
          buffer [j] = func (i + j);
        }
        result.setNb (i, buffer, nb_write);
      }

      free (buffer);
    }

    return result;
  }



}
