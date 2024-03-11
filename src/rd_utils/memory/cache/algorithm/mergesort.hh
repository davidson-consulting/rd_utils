#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include <cstdint>
#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  // UTILITY FUNCTIONS
// Function to print an array
  template <typename T>
  void printArray(T * A, int size) {
    for (int i = 0; i < size; i++) {
      std::cout << A[i] << " ";
    }
    std::cout << std::endl;
  }

#define __INSERT_THRESH 5
#define __swap(x, y) (t = *(x), *(x) = *(y), *(y) = t)

  template <typename T>
  void merge_block (T * a, size_t an, size_t bn) {
    uint32_t *b = a + an, *e = b + bn, *s, t;

    // Return right now if we're done
    if (an == 0 || bn == 0 || !(*b < *(b - 1))) {
      return;
    }

    // Do insertion sort to merge if size of sub-arrays are
    // small enough
    if (an < __INSERT_THRESH && an <= bn) {
      for (uint32_t *p = b, *v; p > a; p--) { // Insert Sort A into B
        for (v = p, s = p - 1; v < e && *v < *s; s = v, v++) {
          __swap(s, v);
        }
      }
      return;
    }

    if (bn < __INSERT_THRESH) {
      for (uint32_t *p = b, *v; p < e;  p++) { // Insert Sort B into A
        for (s = p, v = p - 1; s > a && *s < *v; s = v, v--) {
          __swap(s, v);
        }
      }
      return;
    }

    // Find the pivot points.  Basically this is just
    // finding the point in 'a' where we can swap in the
    // first part of 'b' such that after the swap the last
    // element in 'a' will be less than or equal to the
    // least element in 'b'
    uint32_t *pa = a, *pb = b;
    for (s = a; s < b && pb < e; s++) {
      if (*pb < *pa) {
        pb++;
      } else {
        pa++;
      }
    }

    pa += b - s;

    // Swap first part of b with last part of a
    for (uint32_t *la = pa, *fb = b; la < b; la++, fb++) {
      __swap(la, fb);
    }

    // Now merge the two sub-array pairings
    merge_block (a, pa - a, pb - b);
    merge_block (b, pb - b, e - pb);
  } // merge_array_inplace

#undef __swap
#undef __INSERT_THRESH

  template <typename T>
  void merge_sort_block (T * a, size_t n) {
    size_t m = (n + 1) / 2;

    // Sort first and second halves
    if (m > 1) {
      merge_sort_block (a, m);
    }

    if (n - m > 1) {
      merge_sort_block (a + m, n - m);
    }

    merge_block (a, m, n - m);
  }

  template <typename T>
  void merge (T * buffer, uint32_t BLK_SIZE, collection::CacheArray<T> & arr, size_t low, size_t an, size_t bn) {
    static float all = 0;
    size_t pb = low + an, end = low + an + bn, pa = low;
    uint32_t endA = low + an;
    if (an == 0 || bn == 0) {
      return;
    }

    if (!(arr.get (pb) < arr.get (pb - 1))) return;

    if (an + bn <= BLK_SIZE) {
      auto nb_read = an + bn < BLK_SIZE ? an + bn : BLK_SIZE;
      arr.getNb (low, buffer, BLK_SIZE);
      merge_block (buffer, an, bn);
      arr.setNb (low, buffer, BLK_SIZE);
      return;
    }

    auto HALF_SIZE = BLK_SIZE / 2;
    T * bufferA = buffer, *bufferB = buffer + (HALF_SIZE);
    auto maxReadA = (an <= HALF_SIZE) ? an : HALF_SIZE;
    auto maxReadB = (bn <= HALF_SIZE) ? bn : HALF_SIZE;

    size_t s = low;
    arr.getNb (pa, bufferA, maxReadA);
    arr.getNb (pb, bufferB, maxReadB);

    uint32_t jA = 0, jB = 0;
    T *valA = bufferA, *valB = bufferB;
    for (; s < endA && pb < end; s ++) {
      if (*valB < *valA) {
        pb++;
        jB++;
        if (jB < HALF_SIZE) {
          valB ++;
        } else {
          auto nb_read = (end - pb) < HALF_SIZE ? (end - pb) : HALF_SIZE;
          arr.getNb (pb, bufferB, nb_read);
          jB = 0;
          valB = bufferB;
        }
      } else {
        pa++;
        jA++;
        if (jA < HALF_SIZE) {
          valA ++;
        } else {
          auto nb_read = (endA - pa) < HALF_SIZE ? (endA - pa) : HALF_SIZE;
          arr.getNb (pa, bufferA, nb_read);
          jA = 0;
          valA = bufferA;
        }
      }
    }

    pa += endA - s;

    // Swap first part of b with last part of a
    for (uint32_t la = pa, fb = endA; la < endA; ) {
      auto nb_read = (endA - la) < (HALF_SIZE) ? (endA - la) : (HALF_SIZE);
      uint32_t startA = la, startB = fb;
      arr.getNb (startA, bufferA, nb_read);
      arr.getNb (startB, bufferB, nb_read);

      for (uint32_t j = 0 ; j < nb_read ; j++) {
        T r = bufferA [j];
        bufferA [j] = bufferB [j];
        bufferB [j] = r;
        la ++;
        fb ++;
      }

      arr.setNb (startA, bufferA, nb_read);
      arr.setNb (startB, bufferB, nb_read);
    }

    merge (buffer, BLK_SIZE, arr, low, pa - low, pb - (low + an));
    merge (buffer, BLK_SIZE, arr, low + an, pb - (low + an), end - pb);
  } // merge_array_inplace


  template <typename T>
  void merge_sort (T * buffer, uint32_t BLK_SIZE, collection::CacheArray<T> & arr, uint32_t low, uint32_t n) {
    size_t m = (n + 1) / 2;
    if (m > 1) {
      if (m <= BLK_SIZE) {
        arr.getNb (low, buffer, m);
        merge_sort_block (buffer, m);
        arr.setNb (low, buffer, m);
      } else {
        merge_sort (buffer, BLK_SIZE, arr, low, m);
      }
    }

    if (n - m > 1) {
      if (n - m <= BLK_SIZE) {
        arr.getNb (low + m, buffer, n - m);
        merge_sort_block (buffer, n - m);
        arr.setNb (low + m, buffer, n - m);
      } else {
        merge_sort (buffer, BLK_SIZE, arr, low + m, n - m);
      }
    }

    if (n <= BLK_SIZE) {
      auto nb_read = n < BLK_SIZE ? n : BLK_SIZE;
      arr.getNb (low, buffer, nb_read);
      merge_block (buffer, m, n - m);
      arr.setNb (low, buffer, nb_read);
    } else {
      merge (buffer, BLK_SIZE, arr, low, m, n - m);
    }
  }

  template <typename T>
  void merge_sort (collection::CacheArray<T> & a) {
    T * buffer = reinterpret_cast <T*> (malloc (ARRAY_BUFFER_SIZE * sizeof (T)));
    merge_sort (buffer, ARRAY_BUFFER_SIZE, a, 0, a.len ());
    free (buffer);
  }

  template <typename T>
  void merge_sort (std::vector<T> & a) {
    merge_sort_block (a.data (), a.size ());
  }

}
