#pragma once

#include <cstdint>
#include <iostream>
#include <list>

namespace rd_utils::memory::cache {

  struct MemorySegment {
    uint64_t offset;
    uint64_t size;
  };

  class Block {
  private:

    // The address of the block
    uint64_t _addr;

    // The size of the block
    uint64_t _total_size;

    // The free memory segment in the block
    std::list <MemorySegment> _segments;

  public:

    Block (uint64_t addr, uint64_t total_size);

    /**
     * Reserve a segment of the memory
     * @returns: the offset in the block
     */
    bool allocate (uint64_t size, MemorySegment & seg);

    /**
     * Free a segment of memory
     * @returns: true iif succeed
     */
    void free (MemorySegment);

    ~Block ();


    friend std::ostream & operator<< (std::ostream & s, const rd_utils::memory::cache::Block & b);

  private:

    /**
     * Merge the segment to previous and next if touching
     */
    void merge (std::list<MemorySegment>::iterator it);


  };

}

std::ostream & operator<< (std::ostream & s, const rd_utils::memory::cache::MemorySegment & b);
