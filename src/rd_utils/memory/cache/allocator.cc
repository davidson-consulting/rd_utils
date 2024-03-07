#include "allocator.hh"
#include <cstring>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/utils/log.hh>
#include "free_list.hh"
#include <sys/time.h>

namespace rd_utils::memory::cache {

#define BLOCK_SIZE 1024 * 1024 * 4
#define NB_BLOCKS 10 // 1GB

  Allocator Allocator::__GLOBAL__ (NB_BLOCKS * (uint64_t) BLOCK_SIZE, BLOCK_SIZE);

  concurrency::mutex Allocator::__GLOBAL_MUTEX__;

  Allocator::Allocator (uint64_t totalSize, uint32_t blockSize) :
    _max_blocks (totalSize / blockSize)
    , _block_size (blockSize)
    , _max_allocable (blockSize - sizeof (free_list_instance) - sizeof (uint32_t))
  {}

  Allocator& Allocator::instance () {
    return __GLOBAL__;
  }

  /**
   * ============================================================================
   * ============================================================================
   * =============================    ALLOCATIONS   =============================
   * ============================================================================
   * ============================================================================
   * */

  bool Allocator::allocate (uint32_t size, AllocatedSegment & alloc, bool newBlock) {
    auto realSize = free_list_real_size (size);
    if (realSize > this-> _max_allocable) {
      LOG_ERROR ("Cannot allocate more than ", this-> _max_allocable, "B at a time");
      return false;
    }

    uint32_t offset;
    if (!newBlock) {
      for (auto & [addr, mem] : this-> _loaded) {
        auto & bl = this-> _blocks [addr - 1];
        if (bl.max_size >= realSize) {
          if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
            bl.max_size = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
            timeval start;
            gettimeofday (&start, NULL);
            bl.lru = (start.tv_sec % 1000) * 1000 + start.tv_usec;
            alloc = {.blockAddr = addr, .offset = offset};
            return true;
          }
        }
      }

      for (size_t addr = 1 ; addr <= this-> _blocks.size () ; addr++) {
        auto & bl = this-> _blocks [addr - 1];
        if (bl.max_size >= realSize) { // no need to load a block whose size is not big enough for the allocation
          auto mem = this-> load (addr);
          if (free_list_allocate (mem, size, offset)) {
            bl.max_size = free_list_max_size (mem);
            alloc = {.blockAddr = (uint32_t) addr, .offset = offset};
            return true;
          }
        }
      }
    }

    uint32_t addr;
    auto mem = this-> allocateNewBlock (addr);
    if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
      auto & bl = this-> _blocks [addr - 1];
      bl.max_size = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));

      timeval start;
      gettimeofday (&start, NULL);
      bl.lru = (start.tv_sec % 1000) * 1000 + start.tv_usec;

      alloc = {.blockAddr = addr, .offset = offset};
      return true;
    } else {
      LOG_ERROR ("Failed to allocate ", size, ", possible corruption ?");
      return false;
    }
  }

  bool Allocator::allocateSegments (uint64_t size, AllocatedSegment & rest, uint32_t & fstBlock, uint32_t & nbBlocks, uint32_t & blockSize) {
    AllocatedSegment seg;
    bool fst = true;
    nbBlocks = 0;

    while (size >= this-> _max_allocable) {
      auto toAlloc = this-> _max_allocable - sizeof (uint32_t);
      blockSize = this-> _max_allocable;
      this-> allocate (toAlloc, seg, true);
      if (fst) { fstBlock = seg.blockAddr; fst = false; }
      nbBlocks += 1;
      size -= toAlloc;
    }

    if (size > 0) {
      this-> allocate (size, rest);
    }

    return true;
  }

  void Allocator::free (AllocatedSegment alloc) {
    auto mem = this-> load (alloc.blockAddr);
    free_list_free (mem, alloc.offset);

    auto & bl = this-> _blocks [alloc.blockAddr - 1];
    bl.max_size = free_list_max_size (mem);

    if (free_list_empty (mem)) {
      this-> freeBlock (alloc.blockAddr);
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
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }
    memcpy (data, mem + alloc.offset + offset, size);
  }

  void Allocator::read_8 (AllocatedSegment alloc, uint64_t * data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }

    auto z = reinterpret_cast<uint64_t*> (mem + alloc.offset + offset);
    *data = *z;
  }

  void Allocator::read_4 (AllocatedSegment alloc, uint32_t * data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }

    auto z = reinterpret_cast<uint32_t*> (mem + alloc.offset + offset);
    *data = *z;
  }

  void Allocator::read_2 (AllocatedSegment alloc, uint16_t * data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }

    auto z = reinterpret_cast<uint16_t*> (mem + alloc.offset + offset);
    *data = *z;
  }

  void Allocator::read_1 (AllocatedSegment alloc, uint8_t * data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }

    auto z = reinterpret_cast<uint8_t*> (mem + alloc.offset + offset);
    *data = *z;
  }

  void Allocator::write (AllocatedSegment alloc, const void * data, uint32_t offset, uint32_t size) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }
    memcpy (mem + alloc.offset + offset, data, size);
  }

  void Allocator::write_8 (AllocatedSegment alloc, uint64_t data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }
    auto z = reinterpret_cast <uint64_t*> (mem + alloc.offset + offset);
    *z = data;
  }

  void Allocator::write_4 (AllocatedSegment alloc, uint32_t data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }
    auto z = reinterpret_cast <uint32_t*> (mem + alloc.offset + offset);
    *z = data;
  }

  void Allocator::write_2 (AllocatedSegment alloc, uint16_t data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }
    auto z = reinterpret_cast <uint16_t*> (mem + alloc.offset + offset);
    *z = data;
  }

  void Allocator::write_1 (AllocatedSegment alloc, uint8_t data, uint32_t offset) {
    auto mem = this-> _blocks [alloc.blockAddr - 1].mem;
    if (mem == nullptr) {
      mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
    }
    auto z = reinterpret_cast <uint8_t*> (mem + alloc.offset + offset);
    *z = data;
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
      // this-> printLoaded ();
      this-> evictSome (1);
      //this-> printLoaded ();
    }

    auto mem = new uint8_t [this-> _block_size];
    free_list_create (reinterpret_cast<free_list_instance*> (mem), this-> _block_size);

    addr = this-> _blocks.size () + 1;

    timeval start;
    gettimeofday (&start, NULL);
    uint32_t lru = (start.tv_sec % 1000) * 1000 + start.tv_usec;

    BlockInfo info = {.mem = mem, .lru = lru, .max_size = this-> _max_allocable};
    this-> _blocks.push_back (info);
    this-> _loaded.emplace (addr, mem);

    return mem;
  }

  free_list_instance * Allocator::load (uint32_t addr) {
    auto & memory = this-> _blocks [addr - 1];

    timeval start;
    gettimeofday (&start, NULL);
    auto lru = (start.tv_sec % 1000) * 1000 + start.tv_usec;

    if (memory.mem == nullptr) {
      if (this-> _loaded.size () == this-> _max_blocks) {
        //this-> printLoaded ();
        this-> evictSome (1);
        //this-> printLoaded ();
      }

      uint8_t * out = new uint8_t [this-> _block_size];
      if (!this-> _perister.load (addr, out, this-> _block_size)) {
        free_list_create (reinterpret_cast <free_list_instance*> (out), this-> _block_size);
      }

      this-> _loaded.emplace (addr, out);

      memory.lru = lru;
      memory.mem = out;

      return (free_list_instance*) out;
    } else {
      memory.lru = lru;
      return (free_list_instance*) (memory.mem);
    }
  }

  void Allocator::evictSome (uint32_t nb) {
    for (size_t i = 0 ; i < nb ; i++) {
      auto min = time (NULL) + 10;
      uint32_t addr = 0;

      for (auto & [it, ld_mem] : this-> _loaded) {
        auto & bl = this-> _blocks [it - 1];
        if (bl.lru < min) {
          addr = it;
          min = bl.lru;
        }
      }

      if (addr != 0) {
        auto mem = this-> _loaded [addr];
        this-> _perister.save (addr, mem, this-> _block_size);
        delete [] mem;
        this-> _loaded.erase (addr);
        this-> _blocks [addr - 1].mem = nullptr;
      }
      else {
        concurrency::timer::sleep (0.1);
        evictSome (nb - i);
        return ;
      }
    }
  }

  void Allocator::freeBlock (uint32_t addr) {
    auto & bl = this-> _blocks [addr - 1];
    if (bl.mem != nullptr) {
      delete [] bl.mem;
      bl.mem = nullptr;
    } else {
      this-> _perister.erase (addr);
    }

    this-> _loaded.erase (addr);
    while (this-> _blocks.size () != 0) {
      auto & bl = this-> _blocks.back ();
      if (bl.max_size == this-> _max_allocable) {
        if (bl.mem != nullptr) {
          delete [] bl.mem;
          bl.mem = nullptr;
        }

        this-> _blocks.pop_back ();
      } else break;
    }
  }

  void Allocator::printLoaded () const {
    std::cout << "=============" << std::endl;
    for (auto & it : this-> _loaded) {
      auto info = this-> _blocks [it.first - 1];
      std::cout << it.first << " " << ((uint32_t*) (it.second)) << " " << info.lru << std::endl;
    }
    std::cout << "=============" << std::endl;
  }

  const BlockPersister & Allocator::getPersister () const {
    return this-> _perister;
  }

  std::ostream & operator<< (std::ostream & s, rd_utils::memory::cache::Allocator & alloc) {
    s << "Allocator {\n";
    for (size_t addr = 1 ; addr <= alloc._blocks.size () ; addr++) {
      auto & info = alloc._blocks [addr - 1];
      s << "\tBLOCK(" << addr << ", [" << info.lru << "," << info.max_size << "]) {\n";
      free_list_instance * inst = reinterpret_cast <free_list_instance*> (info.mem);
      if (inst == nullptr) {
        inst = alloc.load (addr);
      }

      if (inst != nullptr) {
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
      } else {
        s << "EMPTY" << std::endl;
      }
      s << "\t}\n";
    }
    s << "}";
    return s;
  }

}
