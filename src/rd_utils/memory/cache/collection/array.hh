
#pragma once

#include <rd_utils/memory/cache/allocator.hh>
#include <rd_utils/utils/_.hh>

namespace rd_utils::memory::cache::collection {

  template <typename T>
  class CacheArray {
  private:

    // The memory segment of the array
    AllocatedSegment _rest;

    // The index of the first block
    uint32_t _fstBlockAddr;

    // The nb of blocks allocated especially for this array
    int32_t _nbBlocks;

    // The size of the array
    uint32_t _size;

    // The size per block (without considering the rest)
    uint32_t _sizePerBlock;

    uint32_t _sizeDividePerBlock;

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
      uint32_t nbBl;
      Allocator::instance ().allocateSegments (size * sizeof (T), this-> _rest, this-> _fstBlockAddr, nbBl, this-> _sizePerBlock);
      if (nbBl == 0) {
        this-> _nbBlocks = -1;
        this-> _sizeDividePerBlock = 1;
        this-> _sizePerBlock = 0;
      }
      else {
        this-> _nbBlocks = nbBl;
        this-> _sizeDividePerBlock = this-> _sizePerBlock;
      }
    }

    /**
     * Access an element in the array as a lvalue
     */
    inline void set (uint32_t i, const T & val) {
      uint32_t absolute = i * sizeof (T);
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
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

      int32_t index = absolute / (this-> _sizeDividePerBlock);
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

    inline void setNb (uint32_t i, T * buffer, uint32_t nb) {
      uint32_t absolute = i * sizeof (T);
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
      uint32_t offset = absolute - (index * this-> _sizePerBlock);
      if (index <= this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = sizeof (free_list_instance) + sizeof (uint32_t);

        auto end = nb * sizeof (T) + offset;
        if (end > this-> _sizePerBlock) {
          auto rest = (end - this-> _sizePerBlock) / sizeof (T);
          nb = nb - rest;
          setNb (i + nb, buffer + nb, rest);
        }

        Allocator::instance ().write (seg, buffer, offset, sizeof (T) * nb);
      } else {
        Allocator::instance ().write (seg, buffer, offset, sizeof (T) * nb);
      }
    }

    inline void getNb (uint32_t i, T * buffer, uint32_t nb) {
      uint32_t absolute = i * sizeof (T);
      AllocatedSegment seg = this-> _rest;

      int32_t index = absolute / (this-> _sizeDividePerBlock);
      uint32_t offset = absolute - (index * this-> _sizePerBlock);
      if (index <= this-> _nbBlocks) {
        seg.blockAddr = index + this-> _fstBlockAddr;
        seg.offset = sizeof (free_list_instance) + sizeof (uint32_t);

        auto end = nb * sizeof (T) + offset;
        if (end > this-> _sizePerBlock) {
          auto rest = (end - this-> _sizePerBlock) / sizeof (T);
          nb = nb - rest;
          getNb (i + nb, buffer + nb, rest);
        }

        Allocator::instance ().read (seg, buffer, offset, sizeof (T) * nb);
      } else {
        Allocator::instance ().read (seg, buffer, offset, sizeof (T) * nb);
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

template <typename T>
std::ostream& operator << (std::ostream & s, rd_utils::memory::cache::collection::CacheArray<T> & array) {
  uint32_t BLK_SIZE = 8192;
  T val [BLK_SIZE];
  auto len = array.len ();
  int z = 0;
  s << "[";
  for (uint32_t i = 0 ; i < len ; i += BLK_SIZE) {
    auto nb_read = len - i >= BLK_SIZE ? BLK_SIZE : len - i;
    array.getNb (i, val, nb_read);
    for (uint32_t j = 0 ; j < nb_read ; j++) {
      if (z != 0) s << ", ";
      s << val [j];
      z += 1;
    }
  }
  s << "]";
  return s;
}
