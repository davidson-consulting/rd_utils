#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include "insertsort.hh"
#include <cstdint>
#include "common.hh"

namespace rd_utils::memory::cache::algorithm {





  template <typename T>
  void merge(T * array, T * buffer, int64_t const left, int64_t const mid, int64_t const right) {
    int const subArrayOne = mid - left + 1;
    int const subArrayTwo = right - mid;

    // Create temp arrays
    auto *leftArray = buffer, *rightArray = buffer + subArrayOne;

    // Copy data to temp arrays leftArray[] and rightArray[]
    for (auto i = 0; i < subArrayOne; i++)  leftArray[i] = array[left + i];
    for (auto j = 0; j < subArrayTwo; j++)  rightArray[j] = array[mid + 1 + j];

    auto indexOfSubArrayOne = 0, indexOfSubArrayTwo = 0;
    int indexOfMergedArray = left;

    // Merge the temp arrays back into array[left..right]
    while (indexOfSubArrayOne < subArrayOne && indexOfSubArrayTwo < subArrayTwo) {
      if (leftArray[indexOfSubArrayOne] <= rightArray[indexOfSubArrayTwo]) {
        array[indexOfMergedArray] = leftArray[indexOfSubArrayOne];
        indexOfSubArrayOne++;
      }
      else {
        array[indexOfMergedArray] = rightArray[indexOfSubArrayTwo];
        indexOfSubArrayTwo++;
      }
      indexOfMergedArray++;
    }

    while (indexOfSubArrayOne < subArrayOne) {
      array[indexOfMergedArray] = leftArray[indexOfSubArrayOne];
      indexOfSubArrayOne++;
      indexOfMergedArray++;
    }

    while (indexOfSubArrayTwo < subArrayTwo) {
      array[indexOfMergedArray] = rightArray[indexOfSubArrayTwo];
      indexOfSubArrayTwo++;
      indexOfMergedArray++;
    }
  }

  template <typename T>
  void merge_sort(T * array, T * buffer, int64_t const begin, int64_t const end) {
    if (begin >= end) return;

    if (end - begin < 16) {
      insert_sort (array, begin, end);
      return;
    }

    int mid = begin + (end - begin) / 2;
    merge_sort(buffer, array, begin, mid);
    merge_sort(buffer, array, mid + 1, end);
    merge (array, buffer, begin, mid, end);
  }

  template <typename T>
  void merge_sort (std::vector<T> & v) {
    T * buffer = new T[v.size ()];
    merge_sort (v.data (), buffer, 0, v.size () - 1);
    delete [] buffer;
  }

}
