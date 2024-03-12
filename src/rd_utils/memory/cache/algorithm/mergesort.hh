#pragma once

#include <rd_utils/memory/cache/collection/array.hh>
#include "insertsort.hh"
#include <cstdint>
#include "common.hh"

namespace rd_utils::memory::cache::algorithm {

  template <typename T>
  void merge(T * buffer, T * buffer2, collection::CacheArray<T> * a, collection::CacheArray<T> * aux, int64_t left, int64_t mid, int64_t right) {
    auto HALF_SIZE = ARRAY_BUFFER_SIZE / 2;

    int64_t lSub = mid - left;
    int64_t rSub = right - mid;

    { // merging in the aux array
      auto pusher = aux-> pusher (0, buffer, ARRAY_BUFFER_SIZE);

      auto lPuller = a-> puller (left, buffer2, HALF_SIZE);
      auto rPuller = a-> puller (mid, buffer2 + HALF_SIZE, HALF_SIZE);

      lPuller.next ();
      rPuller.next ();


      int64_t lIndex = 0, rIndex = 0;
      while (lIndex < lSub && rIndex < rSub) {
        if (lPuller.current () <= rPuller.current ()) {
          pusher.push (lPuller.current ());
          lPuller.next ();
          lIndex ++;
        } else {
          pusher.push (rPuller.current ());
          rPuller.next ();
          rIndex ++;
        }
      }

      while (lIndex < lSub) {
        pusher.push (lPuller.current ());
        lPuller.next ();
        lIndex ++;
      }

      while (rIndex < rSub) {
        pusher.push (rPuller.current ());
        rPuller.next ();
        rIndex ++;
      }
    } // pusher.commit ();

    // rewriting the result in the original array
    a-> copy (left, aux-> slice (0, right - left), buffer, ARRAY_BUFFER_SIZE);
  }

  template <typename T>
  void merge_sort (T * buffer, T * buffer2, collection::CacheArray<T> * array, collection::CacheArray<T> * aux, int64_t begin, int64_t end) {
    if (end - begin <= ARRAY_BUFFER_SIZE) {
      array-> getNb (begin, buffer, end - begin);
      std::sort (buffer, buffer + (end - begin));
      array-> setNb (begin, buffer, end - begin);
      return ;
    }

    int64_t mid = begin + (end - begin) / 2;
    merge_sort (buffer, buffer2, array, aux, begin, mid);
    merge_sort (buffer, buffer2, array, aux, mid, end);

    merge (buffer, buffer2, array, aux, begin, mid, end);
  }

  template <typename T>
  void merge_sort (collection::CacheArray<T> & array) {
    T * buffer = new T [ARRAY_BUFFER_SIZE];
    T * buffer2 = new T [ARRAY_BUFFER_SIZE];
    {
      collection::CacheArray<T> aux (array.len ());
      merge_sort (buffer, buffer2, &array, &aux, 0, array.len ());
    } // free (aux)

    delete [] buffer;
    delete [] buffer2;
  }

  template <typename T>
  void merge_block (T * array, T * buffer, int64_t const left, int64_t const mid, int64_t const right) {
    int const subArrayOne = mid - left;
    int const subArrayTwo = right - mid;

    // Create temp arrays
    auto *leftArray = buffer, *rightArray = buffer + subArrayOne;

    // Copy data to temp arrays leftArray[] and rightArray[]
    for (auto i = 0; i < subArrayOne; i++)  leftArray[i] = array[left + i];
    for (auto j = 0; j < subArrayTwo; j++)  rightArray[j] = array[mid + j];

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
  void merge_sort_block (T * array, T * buffer, int64_t const begin, int64_t const end) {
    if (begin >= end) return;
    if (end - begin <= 1) return;
    // if (end - begin < 16) {
    //   insert_sort (array, begin, end);
    //   return;
    // }

    int mid = begin + (end - begin) / 2;
    merge_sort_block (array, buffer, begin, mid);
    merge_sort_block (array, buffer, mid, end);
    merge_block (array, buffer, begin, mid, end);
  }

  template <typename T>
  void merge_sort_vec (std::vector<T> & v) {
    T * buffer = new T [v.size ()];
    merge_sort_block (v.data (), buffer, 0, v.size ());
    delete [] buffer;
  }

}
