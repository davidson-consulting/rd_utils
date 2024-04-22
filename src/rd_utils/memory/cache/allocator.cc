#include "allocator.hh"
#include <cstring>
#include <rd_utils/concurrency/timer.hh>
#include <rd_utils/utils/log.hh>
#include "free_list.hh"
#include <sys/time.h>

namespace rd_utils::memory::cache {

  using namespace rd_utils::memory::cache::remote;

#define BLOCK_SIZE 1024 * 1024 * 4
#define NB_BLOCKS 1024 // 1GB


  Allocator Allocator::__GLOBAL__ (NB_BLOCKS, BLOCK_SIZE);

  concurrency::mutex Allocator::__GLOBAL_MUTEX__;

  /**
   * ============================================================================
   * ============================================================================
   * ================================    CTORS   ================================
   * ============================================================================
   * ============================================================================
   * */

  Allocator::Allocator (uint32_t nbBlocks, uint32_t blockSize) {
    this-> configure (nbBlocks, blockSize);
  }

  Allocator& Allocator::instance () {
    return __GLOBAL__;
  }

  void Allocator::configure (uint32_t nbBlocks, uint32_t blockSize) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      if (this-> _blocks.size () != 0) {
        throw std::runtime_error ("Cannot change size when there are already allocations");
      }

      this-> _max_blocks = nbBlocks;
      this-> _block_size = blockSize;
      this-> _max_allocable = blockSize - ALLOC_HEAD_SIZE;
      if (this-> _persister != nullptr) {
        delete this-> _persister;
        this-> _persister = nullptr;
      }

      this-> _persister = new LocalPersister ();
    }
  }

  void Allocator::configure (uint32_t nbBlocks, uint32_t blockSize, net::SockAddrV4 addr) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      if (this-> _blocks.size () != 0) {
        throw std::runtime_error ("Cannot change size when there are already allocations");
      }

      this-> _max_blocks = nbBlocks;
      this-> _block_size = blockSize;
      this-> _max_allocable = blockSize - ALLOC_HEAD_SIZE;
      if (this-> _persister != nullptr) {
        delete this-> _persister;
        this-> _persister = nullptr;
      }

      this-> _persister = new RemotePersister (addr);
    }
  }

  void Allocator::dispose () {
    if (this-> _persister != nullptr) {
      delete this-> _persister;
      this-> _persister = nullptr;
    }
  }

  Allocator::~Allocator () {
    this-> dispose ();
  }

  /**
   * ============================================================================
   * ============================================================================
   * ============================      GETTERS      =============================
   * ============================================================================
   * ============================================================================
   * */

  const BlockPersister & Allocator::getPersister () const {
    return *this-> _persister;
  }


  uint32_t Allocator::getMaxNbLoadable () const {
    return this-> _max_blocks;
  }

  uint32_t Allocator::getNbLoaded () const {
    return this-> _loaded.size ();
  }

  uint32_t Allocator::getMaxAllocable () const {
    return this-> _max_allocable;
  }

  uint32_t Allocator::getNbStored () const {
    if (this-> _loaded.size () < this-> _blocks.size ()) {
      return this-> _blocks.size () - this-> _loaded.size ();
    }

    return 0;
  }

  uint32_t Allocator::getMinNbStorable () const {
    if (this-> _max_blocks < this-> _blocks.size ()) {
      return this-> _blocks.size () - this-> _max_blocks;
    }

    return 0;
  }

  uint32_t Allocator::getNbAllocated () const {
    return this-> _blocks.size ();
  }

  uint32_t Allocator::getUniqLoaded () const {
    return this-> _uniqLoads;
  }

  /**
   * ============================================================================
   * ============================================================================
   * ============================      SETTERS      =============================
   * ============================================================================
   * ============================================================================
   * */

  void Allocator::resize (uint32_t nbBlocks) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      if (nbBlocks < 2) this-> _max_blocks = 2;
      else this-> _max_blocks = nbBlocks;
      while (this-> _loaded.size () > this-> _max_blocks) {
        this-> evictSome (this-> _loaded.size () - this-> _max_blocks);
      }
    }
  }

  void Allocator::resetUniqCounter () {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      this-> _lruStamp = this-> _lastLRU;
      this-> _uniqLoads = 0;
    }
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
        if (bl.maxSize >= realSize) {
          if (free_list_allocate (reinterpret_cast <free_list_instance*> (mem), size, offset)) {
            bl.maxSize = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
            bl.lru = this-> _lastLRU++;
            alloc = {.blockAddr = addr, .offset = offset};
            if (lock) __GLOBAL_MUTEX__.unlock ();
            return true;
          }
        }
      }

      for (size_t addr = 1 ; addr <= this-> _blocks.size () ; addr++) {
        auto & bl = this-> _blocks [addr - 1];
        if (bl.maxSize >= realSize) { // no need to load a block whose size is not big enough for the allocation
          auto mem = this-> load (addr);
          if (free_list_allocate (mem, size, offset)) {
            bl.maxSize = free_list_max_size (mem);
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
      bl.maxSize = free_list_max_size (reinterpret_cast <free_list_instance*> (mem));
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

  bool Allocator::allocateSegments (uint32_t elemSize, uint64_t size, AllocatedSegment & rest, uint32_t & fstBlock, uint32_t & nbBlocks, uint32_t & blockSize) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      AllocatedSegment seg;
      bool fst = true;
      nbBlocks = 0;

      while (size >= this-> _max_allocable) {
        auto allocable = this-> _max_allocable - sizeof (uint32_t);
        auto toAlloc = allocable - (allocable % elemSize);

        blockSize = toAlloc;
        this-> allocate (toAlloc, seg, true, false);
        if (fst) { fstBlock = seg.blockAddr; fst = false; }
        nbBlocks += 1;
        size -= toAlloc;
      }

      while (size > 0) {
        auto allocable = size;
        auto toAlloc = allocable - (allocable % elemSize);

        this-> allocate (size, rest, false, false);
        size -= toAlloc;
      }
    }

    return true;
  }

  void Allocator::free (AllocatedSegment alloc) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto mem = this-> load (alloc.blockAddr);
      free_list_free (mem, alloc.offset);

      auto & bl = this-> _blocks [alloc.blockAddr - 1];
      bl.maxSize = free_list_max_size (mem);

      if (free_list_empty (mem)) {
        this-> freeBlock (alloc.blockAddr);
      }
    }
  }

  void Allocator::freeFast (uint32_t blockAddr) {
    auto & bl = this-> _blocks [blockAddr - 1];
    bl.maxSize = this-> _max_allocable;
    this-> freeBlock (blockAddr);
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
      auto mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
      memcpy (data, mem + alloc.offset + offset, size);
    }
  }

  void Allocator::write (AllocatedSegment alloc, const void * data, uint32_t offset, uint32_t size) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto mem = reinterpret_cast <uint8_t*> (this-> load (alloc.blockAddr));
      memcpy (mem + alloc.offset + offset, data, size);
    }
  }

  void Allocator::copy (AllocatedSegment left, AllocatedSegment right, uint32_t size) {
    WITH_LOCK (__GLOBAL_MUTEX__) {
      auto lMem = reinterpret_cast <uint8_t*> (this-> load (left.blockAddr));
      auto rMem = reinterpret_cast <uint8_t*> (this-> load (right.blockAddr));

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
      this-> evictSome (std::min (1, std::max (1, NB_BLOCKS - 1)));
    }

    mem = new uint8_t[this-> _block_size];
    memset (mem, 0, this-> _block_size);
    free_list_create (reinterpret_cast<free_list_instance*> (mem), this-> _block_size);
    addr = this-> _blocks.size () + 1;
    uint32_t lru = this-> _lastLRU++;
    BlockInfo info = {.mem = mem, .lru = lru, .maxSize = this-> _max_allocable};
    this-> _blocks.push_back (info);
    this-> _loaded.emplace (addr, mem);
    this-> _uniqLoads += 1;

    return mem;
  }

  free_list_instance * Allocator::load (uint32_t addr) {
    auto & memory = this-> _blocks [addr - 1];
    auto lru = this-> _lastLRU++;

    if (memory.mem == nullptr) {
      uint8_t * out = nullptr;

      if (this-> _loaded.size () == this-> _max_blocks) {
        this-> evictSome (std::min (1, std::max (1, NB_BLOCKS - 1)));
      }

      out = new uint8_t [this-> _block_size];
      this-> _persister-> load (addr, out, this-> _block_size);
      this-> _loaded.emplace (addr, out);

      if (memory.lru < this-> _lruStamp) {
        this-> _uniqLoads += 1;
      }

      memory.lru = lru;
      memory.mem = out;

      return (free_list_instance*) out;
    } else {
      if (memory.lru < this-> _lruStamp) {
        this-> _uniqLoads += 1;
      }

      memory.lru = lru;
      return (free_list_instance*) (memory.mem);
    }
  }

  void Allocator::evictSome (uint32_t nb) {
    // We don't lock the mutex, we can only enter here if we are already locked

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
        this-> _persister-> save (addr, mem, this-> _block_size);
        this-> _loaded.erase (addr);
        this-> _blocks [addr - 1].mem = nullptr;
        delete [] mem;
        mem = nullptr;
      }
      else {
        concurrency::timer::sleep (0.1);
        evictSome (nb - i);
      }
    }
  }

  void Allocator::freeBlock (uint32_t addr) {
    // No need to lock, only called within lock
    auto & bl = this-> _blocks [addr - 1];
    if (bl.mem != nullptr) {
      delete [] bl.mem;
      bl.mem = nullptr;
    }

    // std::cout << "Freeing block : " << addr << std::endl;
    this-> _persister-> erase (addr);
    this-> _loaded.erase (addr);

    while (this-> _blocks.size () != 0) {
      auto & bl = this-> _blocks.back ();
      if (bl.maxSize == this-> _max_allocable) {
        if (bl.mem != nullptr) {
          delete [] bl.mem;
          bl.mem = nullptr;
        }

        this-> _blocks.pop_back ();
      } else break;
    }
  }

  /**
   * ============================================================================
   * ============================================================================
   * =============================      MISCS      ==============================
   * ============================================================================
   * ============================================================================
   * */

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


  std::ostream & operator<< (std::ostream & s, rd_utils::memory::cache::Allocator & alloc) {
    s << "Allocator {\n";
    for (size_t addr = 1 ; addr <= alloc._blocks.size () ; addr++) {
      auto & info = alloc._blocks [addr - 1];
      s << "\tBLOCK(" << addr << ", [" << info.lru << "," << info.maxSize << "]) {\n";
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
