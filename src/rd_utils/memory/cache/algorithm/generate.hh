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
      {
        auto p = result.pusher (0, buffer, ARRAY_BUFFER_SIZE);
        for (uint32_t i = 0 ; i < len ; i++) {
          p.push (func (i));
        }
      } // p.commit ();

      free (buffer);
    }

    return result;
  }

  template <typename Z, typename X>
  collection::CacheArray<Z> generate (uint32_t len, X * x, Z (X::*func)(uint32_t)) {
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      {
        auto p = result.pusher (0, buffer, ARRAY_BUFFER_SIZE);
        for (uint32_t i = 0 ; i < len ; i++) {
          p.push ((x->* (func)) (i));
        }
      } // p.commit ();

      free (buffer);
    }

    return result;
  }

  template <typename Z, typename F>
  collection::CacheArray<Z> generate (uint32_t len, F func) {
    collection::CacheArray<Z> result (len);
    if (len > 0) {
      Z * buffer = reinterpret_cast <Z*> (malloc (ARRAY_BUFFER_SIZE * sizeof (Z)));
      {
        auto p = result.pusher (0, buffer, ARRAY_BUFFER_SIZE);
        for (uint32_t i = 0 ; i < len  ; i++) {
          p.push (func (i));
        }
      } // p.commit ();
      free (buffer);
    }

    return result;
  }



}
