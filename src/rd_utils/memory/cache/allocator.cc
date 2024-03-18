#include "allocator.hh"
#include <cstring>
#include <rd_utils/concurrency/timer.hh>
#include "free_list.hh"
#include <sys/time.h>

namespace rd_utils::memory::cache {

#define BLOCK_SIZE 1024 * 1024 * 4
#define NB_BLOCKS 1024 // 1GB


  Allocator Allocator::__GLOBAL__ (NB_BLOCKS * (uint64_t) BLOCK_SIZE, BLOCK_SIZE);

  concurrency::mutex Allocator::__GLOBAL_MUTEX__;

  Allocator::Allocator (uint64_t totalSize, uint32_t blockSize) :
    _max_blocks (totalSize / blockSize)
    , _block_size (blockSize)
    , _max_allocable (blockSize - ALLOC_HEAD_SIZE)
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

  bool Allocator::allocate (uint32_t size, AllocatedSegment & alloc, bool newBlock, bool lock) {
    auto realSize = free_list_real_size (size);
    if (realSize > this-> _max_allocable) {
      //LOG_ERROR ("Cannot allocate more than ", this-> _max_allocable, "B at a time");
      return false;
    }

    uint32_t offset;
    if (!newBlock) {
      if (lock) __GLOBAL_MUTEX__.lock ();
      for (auto & [addr, mem] : this-> _loaded) {
        auto & bl = this-> _blocks [addr - 1];
        if (bl.max_size >= realSize) {
          if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
            bl.max_size = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
            bl.lru = this-> _lastLRU++;
            alloc = {.blockAddr = addr, .offset = offset};
            if (lock) __GLOBAL_MUTEX__.unlock ();
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
            if (lock) __GLOBAL_MUTEX__.unlock ();
            return true;
          }
        }
      }
    }

    uint32_t addr;
    if (lock) __GLOBAL_MUTEX__.lock ();
    auto mem = this-> allocateNewBlock (addr);
    if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
      auto & bl = this-> _blocks [addr - 1];
      bl.max_size = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
      bl.lru = this-> _lastLRU++;

      alloc = {.blockAddr = addr, .offset = offset};
      if (lock) __GLOBAL_MUTEX__.unlock ();
      return true;
    } else {
      if (lock) __GLOBAL_MUTEX__.unlock ();
      //LOG_ERROR ("Failed to allocate ", size, ", possible corruption ?");
      return false;
    }
  }

  bool Allocator::allocateSegments (uint64_t size, AllocatedSegment & rest, uint32_t & fstBlock, uint32_t & nbBlocks, uint32_t & blockSize) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      AllocatedSegment seg;
      bool fst = true;
      nbBlocks = 0;

      while (size >= this-> _max_allocable) {
        auto toAlloc = this-> _max_allocable - sizeof (uint32_t);
        blockSize = toAlloc;
        this-> allocate (toAlloc, seg, true, false);
        if (fst) { fstBlock = seg.blockAddr; fst = false; }
        nbBlocks += 1;
        size -= toAlloc;
      }

      if (size > 0) {
        this-> allocate (size, rest, false, false);
      }
    }

    return true;
  }

  void Allocator::free (AllocatedSegment alloc) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto mem = this-> load (alloc.blockAddr);
      free_list_free (mem, alloc.offset);

      auto & bl = this-> _blocks [alloc.blockAddr - 1];
      bl.max_size = free_list_max_size (mem);

      if (free_list_empty (mem)) {
        this-> freeBlock (alloc.blockAddr);
      }
    }
  }

  void Allocator::free (const std::vector <AllocatedSegment> & segments) {
    std::vector <AllocatedSegment> copy = segments;
    int i = 0;
    for (auto it : segments) { // Start by freeing already loaded blocks
      __GLOBAL_MUTEX__.lock ();
      auto lt = this-> _loaded.find (it.blockAddr) ;
      __GLOBAL_MUTEX__.unlock ();

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
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto & bl = this-> _blocks [alloc.blockAddr - 1];
      auto mem = bl.mem;
      if (mem == nullptr) {
        mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
      }

      memcpy (data, mem + alloc.offset + offset, size);
    }
  }

  void Allocator::write (AllocatedSegment alloc, const void * data, uint32_t offset, uint32_t size) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto & bl =  this-> _blocks [alloc.blockAddr - 1];
      auto mem = bl.mem;
      if (mem == nullptr) {
        mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
      }

      memcpy (mem + alloc.offset + offset, data, size);
    }
  }

  void Allocator::copy (AllocatedSegment left, AllocatedSegment right, uint32_t size) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto & lBl = this-> _blocks [left.blockAddr - 1];
      auto & rBl = this-> _blocks [right.blockAddr - 1];
      auto lMem = lBl.mem;
      auto rMem = rBl.mem;
      if (lMem == nullptr) {
        lMem = reinterpret_cast<uint8_t*> (this-> load (left.blockAddr));
      }
      if (rMem == nullptr) {
        rMem = reinterpret_cast <uint8_t*> (this-> load (right.blockAddr));
      }

      memcpy (rMem + right.offset, lMem + left.offset, size);
    }
  }

  bool Allocator::isLoaded (uint32_t blockAddr) const {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto & bl = this-> _blocks [blockAddr - 1];
      return bl.mem != nullptr;
    }
  }

  /**
   * ===========================================================================
   * ===========================================================================
   * =============================    MANAGEMENT   =============================
   * ===========================================================================
   * ===========================================================================
   * */

  uint8_t * Allocator::allocateNewBlock (uint32_t & addr) {
    uint8_t * mem = nullptr;
    if (this-> _loaded.size () == this-> _max_blocks) {
      mem = this-> evictSome (std::min (1, std::max (1, NB_BLOCKS - 1)));
    } else {
      mem = (uint8_t*) malloc (this-> _block_size);
    }

    free_list_create (reinterpret_cast<free_list_instance*> (mem), this-> _block_size);
    addr = this-> _blocks.size () + 1;
    uint32_t lru = this-> _lastLRU++;
    BlockInfo info = {.mem = mem, .lock = false, .lru = lru, .max_size = this-> _max_allocable};
    this-> _blocks.push_back (info);
    this-> _loaded.emplace (addr, mem);


    return mem;
  }

  free_list_instance * Allocator::load (uint32_t addr) {
    auto & memory = this-> _blocks [addr - 1];
    auto lru = this-> _lastLRU++;

    if (memory.mem == nullptr) {
      uint8_t * out = nullptr;

      if (this-> _loaded.size () == this-> _max_blocks) {
        out = this-> evictSome (std::min (1, std::max (1, NB_BLOCKS - 1)));
      } else {
        out = (uint8_t*) malloc (this-> _block_size);
      }

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

  uint8_t * Allocator::evictSome (uint32_t nb) {
    // We don't lock the mutex, we can only enter here if we are already locked

    for (size_t i = 0 ; i < nb ; i++) {
      auto min = time (NULL) + 10;
      uint32_t addr = 0;

      for (auto & [it, ld_mem] : this-> _loaded) {
        auto & bl = this-> _blocks [it - 1];
        if (bl.lru < min && !bl.lock) {
          addr = it;
          min = bl.lru;
        }
      }

      if (addr != 0) {
        auto mem = this-> _loaded [addr];
        this-> _perister.save (addr, mem, this-> _block_size);
        this-> _loaded.erase (addr);
        this-> _blocks [addr - 1].mem = nullptr;

        if (i != nb - 1) {
          ::free (mem);
          mem = nullptr;
        }
        else {
          return mem;
        }
      }
      else {
        std::cout << "Waiting unlock ?" << std::endl;
        concurrency::timer::sleep (0.1);
        return evictSome (nb - i);
      }
    }

    return nullptr;
  }

  void Allocator::freeBlock (uint32_t addr) {
    // No need to lock, only called within lock
    auto & bl = this-> _blocks [addr - 1];
    if (bl.mem != nullptr) {
      ::free (bl.mem);
      bl.mem = nullptr;
    }

    this-> _perister.erase (addr);
    this-> _loaded.erase (addr);

    while (this-> _blocks.size () != 0) {
      auto & bl = this-> _blocks.back ();
      if (bl.max_size == this-> _max_allocable) {
        if (bl.mem != nullptr) {
          ::free (bl.mem);
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

  void Allocator::printBlocks () const {
    std::cout << "=============" << std::endl;
    uint32_t addr = 1;
    for (auto & info : this-> _blocks) {
      std::cout << addr << " " << ((uint32_t*) (info.mem)) << " " << info.lru << std::endl;
      addr += 1;
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
