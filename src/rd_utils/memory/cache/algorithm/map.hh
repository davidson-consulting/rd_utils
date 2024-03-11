#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>

#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename Z, typename T>
  collection::CacheArray<Z> map (collection::CacheArray<T> & array, Z (*func)(T)) {
    collection::CacheArray<Z> result (array.len ());
    uint32_t len = array.len ();
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      for (uint32_t i = 0 ; i < len ; i += ARRAY_BUFFER_SIZE) {
        auto nb_write = len - i >= ARRAY_BUFFER_SIZE ? ARRAY_BUFFER_SIZE : len - i;
        array.getNb (i, buffer, nb_write);

        for (uint32_t j = 0 ; j < nb_write ; j++) {
          buffer [j] = func (i + j, buffer [j]);
        }

        result.setNb (i, buffer, nb_write);
      }

      free (buffer);
    }

    return result;
  }

  template <typename Z, typename T, typename X>
  collection::CacheArray<Z> map (collection::CacheArray<T> & array, X * x, Z (X::*func)(T)) {
    uint32_t len = array.len ();
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      for (uint32_t i = 0 ; i < len ; i += ARRAY_BUFFER_SIZE) {
        auto nb_write = len - i >= ARRAY_BUFFER_SIZE ? ARRAY_BUFFER_SIZE : len - i;

        array.getNb (i, buffer, nb_write);

        for (uint32_t j = 0 ; j < nb_write ; j++) {
          buffer [j] = (x->* (func)) (i + j, buffer [j]);
        }

        result.setNb (i, buffer, nb_write);
      }

      free (buffer);
    }

    return result;
  }

  template <typename Z, typename T, typename F>
  collection::CacheArray<Z> map (collection::CacheArray<T> & array, F func) {
    uint32_t len = array.len ();
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      for (uint32_t i = 0 ; i < len ; i += ARRAY_BUFFER_SIZE) {
        auto nb_write = len - i >= ARRAY_BUFFER_SIZE ? ARRAY_BUFFER_SIZE : len - i;
        array.getNb (i, buffer, nb_write);

        for (uint32_t j = 0 ; j < nb_write ; j++) {
          buffer [j] = func (i + j, buffer [j]);
        }

        result.setNb (i, buffer, nb_write);
      }

      free (buffer);
    }

    return result;
  }

}
