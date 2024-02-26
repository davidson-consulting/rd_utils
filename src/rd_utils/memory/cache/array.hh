#pragma once

#include "allocator.hh"
#include <rd_utils/utils/_.hh>

namespace rd_utils::memory::cache {

  template <typename T>
  class CacheArray {
  private:

    // The memory segment of the array
    std::vector <AllocatedSegment> _segments;

    // The size of the array
    uint32_t _size;

    // The size per block
    uint32_t _sizePerBlock;

  private:

    struct ProxyValue {
      AllocatedSegment loc;
      uint32_t sub_offset;

      ProxyValue (AllocatedSegment seg, uint32_t sub_offset) :
        loc (seg)
        , sub_offset (sub_offset)
      {}

      void operator=(const T & val) {
        Allocator::instance ()-> write (loc, &val, sub_offset, sizeof (T));
      }
    };


  private:

    CacheArray (const CacheArray<T> &);
    void operator=(const CacheArray<T>&);

  public:

    /**
     * Create a new array of a fixed size
     */
    CacheArray (uint32_t size) :
      _size (size)
    {
      std::vector <uint32_t> sizes;
      if (Allocator::instance ()-> allocateSegments (size * sizeof (T), this-> _segments, sizes)) {
        this-> _sizePerBlock = sizes [0];
      }

      std::cout << sizes << std::endl;
    }

    /**
     * Access an element in the array as a lvalue
     */
    ProxyValue set (uint32_t index) {
      uint32_t offset = 0;
      auto relIndex = this-> getIndex (index * sizeof (T), offset);
      return ProxyValue (this-> _segments [relIndex], offset);
    }

    /**
     * Access an element in the array
     */
    T get (uint32_t index) const {
      char buffer [sizeof (T)];
      uint32_t offset = 0;
      auto relIndex = this-> getIndex (index * sizeof (T), offset);
      auto seg = this-> _segments [relIndex];
      Allocator::instance ()-> read (seg, buffer, offset, sizeof (T));

      return *reinterpret_cast<T*> (buffer);
    }

    uint32_t len () const {
      return this-> _size;
    }

    /**
     * Free the array allocation
     */
    ~CacheArray () {
      if (this-> _segments.size () != 0) {
        Allocator::instance ()-> free (this-> _segments);
      }
    }

  private:

    /**
     * @returns:
     *    - the index of the segment to access
     *    - offset: the offset of the value within the segment
     */
    uint32_t getIndex (uint32_t absolute, uint32_t & offset) const {
      auto index = absolute / (this-> _sizePerBlock);
      offset = absolute - (index * this-> _sizePerBlock);
      return index;
    }

    uint32_t binsearch (uint32_t value) const {
      return this-> _sizes.size () / (1024 * 1024);
    }

  };

}
