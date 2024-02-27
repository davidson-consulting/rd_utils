#pragma once

#include "allocator.hh"
#include <rd_utils/utils/_.hh>

namespace rd_utils::memory::cache {

  template <typename T>
  class CacheArray {
  private:

    // The memory segment of the array
    AllocatedSegment _rest;

    // The index of the first block
    uint32_t _fstBlockAddr;

    // The nb of blocks allocated especially for this array
    uint32_t _nbBlocks;

    // The size of the array
    uint32_t _size;

    // The size per block (without considering the rest)
    uint32_t _sizePerBlock;

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
      Allocator::instance ().allocateSegments (size * sizeof (T), this-> _rest, this-> _fstBlockAddr, this-> _nbBlocks, this-> _sizePerBlock);
    }

    /**
     * Access an element in the array as a lvalue
     */
    inline void set (uint32_t i, const T & val) {
      uint32_t absolute = i * sizeof (T);
      AllocatedSegment seg = this-> _rest;

      auto index = absolute / (this-> _sizePerBlock);
      uint32_t offset = absolute - (index * this-> _sizePerBlock);

      if (index <= this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = sizeof (free_list_instance) + sizeof (uint32_t);
      }

      if constexpr (sizeof (T) == 8) {
        Allocator::instance ().write_8 (seg, *reinterpret_cast<const uint64_t*> (&val), offset);
      } else if constexpr (sizeof (T) == 4) {
        Allocator::instance ().write_4 (seg, *reinterpret_cast<const uint32_t*> (&val), offset);
      } else if constexpr (sizeof (T) == 2) {
        Allocator::instance ().write_2 (seg, *reinterpret_cast<const uint16_t*> (&val), offset);
      } else if constexpr (sizeof (T) == 1) {
        Allocator::instance ().write_1 (seg, *reinterpret_cast<const uint8_t*> (&val), offset);
      } else {
        Allocator::instance ().write (seg, &val, offset, sizeof (T));
      }
    }

    /**
     * Access an element in the array
     */
    inline T get (uint32_t i) const {
      uint32_t absolute = i * sizeof (T);
      AllocatedSegment seg = this-> _rest;

      auto index = absolute / (this-> _sizePerBlock);
      uint32_t offset = absolute - (index * this-> _sizePerBlock);

      if (index <= this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = sizeof (free_list_instance) + sizeof (uint32_t);
      }

      if constexpr (sizeof (T) == 8) {
        uint64_t buffer;
        Allocator::instance ().read_8 (seg, &buffer, offset);
        return *reinterpret_cast<T*> (&buffer);
        return buffer;
      } else if constexpr (sizeof (T) == 4) {
        uint32_t buffer;
        Allocator::instance ().read_4 (seg, &buffer, offset);
        return *reinterpret_cast<T*> (&buffer);
      } else if constexpr (sizeof (T) == 2) {
        uint16_t buffer;
        Allocator::instance ().read_2 (seg, &buffer, offset);
        return *reinterpret_cast<T*> (&buffer);
      } else if constexpr (sizeof (T) == 1) {
        uint8_t buffer;
        Allocator::instance ().read_1 (seg, &buffer, offset);
        return *reinterpret_cast<T*> (&buffer);
      } else {
        char buffer [sizeof (T)];
        Allocator::instance ().read (seg, buffer, offset, sizeof (T));
        return *reinterpret_cast<T*> (buffer);
      }
    }

    inline uint32_t len () const {
      return this-> _size;
    }

    /**
     * Free the array allocation
     */
    ~CacheArray () {
      std::vector <AllocatedSegment> segs;
      if (this-> _rest.blockAddr != 0) { segs.push_back (this-> _rest); }
      for (uint32_t addr = this-> _fstBlockAddr ; addr <= this-> _fstBlockAddr + this-> _nbBlocks ; addr++) {
        segs.push_back ({.blockAddr = addr, .offset = sizeof (free_list_instance) + sizeof (uint32_t)});
      }

      Allocator::instance ().free (segs);
      // Allocator::instance ()-> getPersister ().printInfo ();
    }

  };

}
