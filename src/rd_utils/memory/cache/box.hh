#pragma once

#include "allocator.hh"
#include <rd_utils/utils/log.hh>


namespace rd_utils::memory::cache {

  template <typename T>
  class CacheBox {
  private:

    AllocatedSegment _segment;

  private:

    CacheBox (const CacheBox& other);
    void operator=(const CacheBox & other);

  public:

    /**
     * Create a cachebox to store an object in the hybrid cache (memory / disk)
     * @params:
     *    - allocator: the allocator used to allocate the box
     */
    CacheBox ()
    {
      if (!Allocator::instance ().allocate (sizeof (T), this-> _segment)) {
        LOG_ERROR ("Failed to allocate box");
        exit (-1);
      }
    }

    void write (const T& t) {
      Allocator::instance ().write (this-> _segment, &t, 0, sizeof (T));
    }

    T read () {
      T t;
      Allocator::instance ().read (this-> _segment, &t, 0, sizeof (T));
      return t;
    }

    ~CacheBox () {
      Allocator::instance ().free (this-> _segment);
      this-> _segment = {0};
    }

  };

}
