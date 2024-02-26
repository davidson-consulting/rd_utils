#include "allocator.hh"
#include <cstring>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/utils/log.hh>
#include "free_list.hh"

namespace rd_utils::memory::cache {

#define BLOCK_SIZE 1 * 1024 * 1024
#define NB_BLOCKS 200 // 1GB


  Allocator* Allocator::__GLOBAL__;

  concurrency::mutex Allocator::__GLOBAL_MUTEX__;

  Allocator::Allocator (uint64_t totalSize, uint32_t blockSize) :
    _max_blocks (totalSize / blockSize)
    , _block_size (blockSize)
    , _max_allocable (blockSize - sizeof (free_list_instance) - sizeof (uint32_t))
  {}

  Allocator* Allocator::instance () {
    if (__GLOBAL__ == nullptr) {
      WITH_LOCK (__GLOBAL_MUTEX__) {
        if (__GLOBAL__ == nullptr) {
          // 1 GB, from 1024 blocks of 1MB
          __GLOBAL__ = new Allocator (NB_BLOCKS * BLOCK_SIZE, BLOCK_SIZE);
        }
      }
    }

    return __GLOBAL__;
  }

  /**
   * ============================================================================
   * ============================================================================
   * =============================    ALLOCATIONS   =============================
   * ============================================================================
   * ============================================================================
   * */

  bool Allocator::allocate (uint32_t size, AllocatedSegment & alloc) {
    auto realSize = free_list_real_size (size);
    if (realSize > this-> _max_allocable) {
      LOG_ERROR ("Cannot allocate more than ", this-> _max_allocable, "B at a time");
      return false;
    }

    uint32_t offset;
    for (auto & [it, mem] : this-> _loaded) {
      if (this-> _blocks [it].max_size >= realSize) {
        if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
          auto & bl = this-> _blocks [it];
          bl.max_size = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
          bl.lru = time (NULL);
          alloc = {.blockAddr = it, .offset = offset};
          return true;
        }
      }
    }

    for (auto & [it, info] : this-> _blocks) {
      if (info.max_size >= realSize) { // no need to load a block whose size is not big enough for the allocation
        auto mem = this-> load (it);
        if (free_list_allocate (mem, size, offset)) {
          info.max_size = free_list_max_size (mem);
          alloc = {.blockAddr = it, .offset = offset};
          return true;
        }
      }
    }

    uint32_t addr;
    auto mem = this-> allocateNewBlock (addr);
    if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
      auto & bl = this-> _blocks [addr];
      bl.max_size = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
      bl.lru = time (NULL);

      alloc = {.blockAddr = addr, .offset = offset};
      return true;
    } else {
      LOG_ERROR ("Failed to allocate ", size, ", possible corruption ?");
      return false;
    }
  }

  bool Allocator::allocateSegments (uint64_t size, std::vector <AllocatedSegment> & allocateds, std::vector <uint32_t> & sizes) {
    AllocatedSegment seg;
    while (size >= this-> _max_allocable) {
      auto toAlloc = this-> _max_allocable - sizeof (uint32_t);
      if (!this-> allocate (toAlloc, seg)) {
        this-> free (allocateds);
        return false;
      }

      size -= toAlloc;
      allocateds.push_back (seg);
      sizes.push_back (toAlloc);
    }

    if (size > 0) {
      if (!this-> allocate (size, seg)) {
        this-> free (allocateds);
        return false;
      }

      allocateds.push_back (seg);
      sizes.push_back (size);
    }

    return true;
  }

  void Allocator::free (AllocatedSegment alloc) {
    auto mem = this-> load (alloc.blockAddr);
    free_list_free (mem, alloc.offset);

    if (free_list_empty (mem)) {
      this-> freeBlock (alloc.blockAddr);
    } else {
      auto & bl = this-> _blocks [alloc.blockAddr];
      bl.max_size = free_list_max_size (mem);
    }
  }

  void Allocator::free (const std::vector <AllocatedSegment> & segments) {
    std::vector <AllocatedSegment> copy = segments;
    int i = 0;
    for (auto it : segments) { // Start by freeing already loaded blocks
      auto lt = this-> _loaded.find (it.blockAddr) ;
      if (lt != this-> _loaded.end ()) {
        this-> free (it);
        copy.erase (copy.begin () + i);
      } else {
        i += 1;
      }
    }

    for (auto it : copy) { // free other blocks
      this-> free (it);
    }
  }

  /**
   * =============================================================================
   * =============================================================================
   * =============================    INPUT/OUTPUT   =============================
   * =============================================================================
   * =============================================================================
   * */

  void Allocator::read (AllocatedSegment alloc, void * data, uint32_t offset, uint32_t size) {
    auto mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    memcpy (data, mem + alloc.offset + offset, size);
  }

  void Allocator::write (AllocatedSegment alloc, const void * data, uint32_t offset, uint32_t size) {
    auto mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    memcpy (mem + alloc.offset + offset, data, size);
  }

  /**
   * ===========================================================================
   * ===========================================================================
   * =============================    MANAGEMENT   =============================
   * ===========================================================================
   * ===========================================================================
   * */

  uint8_t * Allocator::allocateNewBlock (uint32_t & addr) {
    if (this-> _loaded.size () == this-> _max_blocks) {
      this-> evictSome (1);
    }

    auto mem = new uint8_t [this-> _block_size];
    free_list_create (reinterpret_cast<free_list_instance*> (mem), this-> _block_size);

    this-> _last_block += 1;
    addr = this-> _last_block;

    BlockInfo info = {.lru = (uint32_t) (time (NULL)), .max_size = this-> _max_allocable};
    this-> _blocks.emplace (addr, info);
    this-> _loaded.emplace (addr, mem);

    return mem;
  }

  free_list_instance * Allocator::load (uint32_t addr) {
    auto memory = this-> _loaded.find (addr);
    if (memory == this-> _loaded.end ()) {
      if (this-> _loaded.size () == this-> _max_blocks) {
        this-> evictSome (1);
      }

      uint8_t * out = new uint8_t [this-> _block_size];

      this-> _perister.load (addr, out, this-> _block_size);
      this-> _loaded.emplace (addr, out);
      this-> _blocks [addr].lru = time (NULL);

      return (free_list_instance*) out;
    } else {
      // this-> _blocks [addr].lru = time (NULL);
      return (free_list_instance*) (memory-> second);
    }
  }

  void Allocator::evictSome (uint32_t nb) {
    for (size_t i = 0 ; i < nb ; i++) {
      auto min = time (NULL) + 10;
      uint32_t addr = 0;

      for (auto & [it, ld_mem] : this-> _loaded) {
        auto bl = this-> _blocks.find (it);
        if (bl-> second.lru < min) {
          addr = it;
          min = bl-> second.lru;
        }
      }

      if (addr != 0) {
        auto mem = this-> _loaded [addr];
        this-> _perister.save (addr, mem, this-> _block_size);
        delete [] mem;
        this-> _loaded.erase (addr);
      }
      else {
        concurrency::timer::sleep (0.1);
        evictSome (nb - i);
        return ;
      }
    }
  }

  void Allocator::freeBlock (uint32_t addr) {
    auto loaded = this-> _loaded.find (addr);
    if (loaded != this-> _loaded.end ()) {
      delete [] loaded-> second;
      this-> _loaded.erase (addr);
    } else {
      this-> _perister.erase (addr);
    }

    this-> _blocks.erase (addr);
  }

  void Allocator::printLoaded () const {
    std::cout << "=============" << std::endl;
    for (auto & it : this-> _loaded) {
      std::cout << it.first << " " << ((uint32_t*) (it.second)) << std::endl;
    }
    std::cout << "=============" << std::endl;
  }


  std::ostream & operator<< (std::ostream & s, rd_utils::memory::cache::Allocator & alloc) {
    s << "Allocator {\n";
    for (auto & [it, info] : alloc._blocks) {
      s << "\tBLOCK(" << it << ", [" << info.lru << "," << info.max_size << "]) {\n";
      auto inst = alloc.load (it);

      auto memory = reinterpret_cast <const uint8_t*> (inst);
      s << "\t\tLIST{";
      int i = 0;
      auto nodeOffset = inst-> head;
      while (nodeOffset != 0) {
        auto node = reinterpret_cast <const rd_utils::memory::cache::free_list_node *> (memory + nodeOffset);
        if (i != 0) { s << ", "; i += 1; }
        s << "(" << nodeOffset << "," << nodeOffset + node-> size << ")";
        nodeOffset = node-> next;
      }
      s << "}\n";
      s << "\t}\n";
    }
    s << "}";
    return s;
  }

}
