#pragma once

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "free_list.hh"
#include <rd_utils/memory/cache/remote/persist.hh>
#include <rd_utils/concurrency/mutex.hh>

namespace rd_utils::memory::cache {

#define ALLOC_HEAD_SIZE (sizeof (free_list_instance) + sizeof (uint32_t))

        struct AllocatedSegment {
                uint32_t blockAddr;
                uint32_t offset;
        };

        struct BlockInfo {
                uint8_t * mem;
                uint32_t lru;
                uint32_t maxSize;
        };

        class Allocator {
        private:

                // The maximum number of blocks that can be loaded at the same time
                uint32_t _max_blocks;

                // The size of a block
                uint32_t _block_size;

                // The maximum size allocable in a block
                uint32_t _max_allocable;

                // The list of block currently in memory (id-> memory)
                std::unordered_map <uint32_t, uint8_t*> _loaded;

                // The list of blocks allocated by the allocator (id -> lru)
                std::vector <BlockInfo> _blocks;

                // The persister to store blocks to disk
                remote::BlockPersister * _persister;

                // Counter used to compute the ordering of loads
                uint32_t _lastLRU = 1;

                // The lru used to compute number of uniq loads
                uint32_t _lruStamp = 0;

                // The number of uniq load between two stamps
                uint32_t _uniqLoads = 0;

        private:

                Allocator (const Allocator&);

                void operator=(const Allocator&);

        public:

                // The global allocator
                static Allocator __GLOBAL__;

                static concurrency::mutex __GLOBAL_MUTEX__;

        public:


                /**
                 * ============================================================================
                 * ============================================================================
                 * ================================    CTORS   ================================
                 * ============================================================================
                 * ============================================================================
                 * */

                /**
                 * @params:
                 *    - nbBlocks: the maximal number of blocks that can be loaded into RAM at the same time
                 *    - blockSize: the size of a block (defining the number of block that can be loaded at the same time)
                 * @info: nbBlocks * blockSize is the maximal amount of memory the allocator can allocate to store blocks into memory
                 * @warning:
                 * ======
                 * the allocator itself uses some memory to store block
                 * informations, this memory is small but still exists making
                 * the actual memory usage of the allocator higher than nbBlocks
                 *  blockSize
                 * ======
                 */
                Allocator (uint32_t nbBlocks, uint32_t blockSize = 1024 * 1024);

                /**
                 * @returns: the global instance of the allocator
                 */
                static Allocator& instance ();

                /**
                 * Configure the size of the allocator
                 * @warning: only works if there is no allocations alive
                 */
                void configure (uint32_t nbBlocks, uint32_t blockSize);

                /**
                 * Configure the size of the allocator
                 * @warning: only works if there is no allocations alive
                 */
                void configure (uint32_t nbBlocks, uint32_t blockSize, net::SockAddrV4 remotePersist);

                /**
                 * Remove all allocated blocks
                 */
                void dispose ();

                /**
                 * this-> dispose ();
                 */
                ~Allocator ();

                /**
                 * ============================================================================
                 * ============================================================================
                 * =============================    ALLOCATIONS   =============================
                 * ============================================================================
                 * ============================================================================
                 * */

                /**
                 * Allocate a segment of memory
                 * @params:
                 *    - size: the size to allocate (must fit in a block)
                 *    - newBlock: true iif we want to force the allocation inside a new block instead of an old one (this can be useful for allocate segments, to ensure that all the blocks are consecutives)
                 * @returns:
                 *    - true iif a segment was found
                 *    - alloc: the allocated segment
                 */
                bool allocate (uint32_t size, AllocatedSegment & alloc, bool newBlock = false, bool lock = true);

                /**
                 * Allocate a list of memory segment (if size does not fit in only one segment)
                 * @returns:
                 *    - true iif a segment was found
                 *    - alloc: the allocated segment
                 */
                bool allocateSegments (uint32_t elemSize, uint64_t size, AllocatedSegment & rest, uint32_t & fstBlock, uint32_t & nbBlocks, uint32_t & blockSize);

                /**
                 * @returns: true if the block /addr/ is loaded
                 */
                bool isLoaded (uint32_t addr) const;

                /**
                 * Free an allocated memory segment
                 */
                void free (AllocatedSegment alloc);

                /**
                 * Free a full block
                 */
                void freeFast (uint32_t blockAddr);

                /**
                 * Free a list of segments
                 */
                void free (const std::vector <AllocatedSegment> & allocs);

                /**
                 * ============================================================================
                 * ============================================================================
                 * ================================     I/O    ================================
                 * ============================================================================
                 * ============================================================================
                 * */

                /**
                 * Read an allocated segment into memory
                 * @params:
                 *    - alloc: the allocated segment to read
                 *    - data: where to write the data
                 *    - offset: the offset of the allocated segment
                 *    - size: the size to read
                 */
                void read (AllocatedSegment alloc, void * data, uint32_t offset, uint32_t size);

                /**
                 * Write onto an allocated segment into memory
                 * @params:
                 *    - alloc: the allocated segment to write
                 *    - data: where to read the data
                 *    - offset: the offset of the allocated segment
                 *    - size: the size to read
                 */
                void write (AllocatedSegment alloc, const void * data, uint32_t offset, uint32_t size);

                /**
                 * Copy data from an allocated segment to another
                 * @params:
                 *    - input: the segment in which we read data
                 *    - right: the segment in which we write data
                 *    - size: the size in bytes to copy
                 */
                void copy (AllocatedSegment input, AllocatedSegment output, uint32_t size);


                /**
                 * ============================================================================
                 * ============================================================================
                 * ============================      GETTERS      =============================
                 * ============================================================================
                 * ============================================================================
                 * */

                /**
                 * @returns: the block persister
                 */
                const remote::BlockPersister & getPersister () const;

                /**
                 * @returns: the maximum number of loaded blocks
                 */
                uint32_t getMaxNbLoadable () const;

                /**
                 * @returns: the number of loaded blocks
                 */
                uint32_t getNbLoaded () const;

                /**
                 * @returns: the maximum number of bits that can be allocated in a single block
                 */
                uint32_t getMaxAllocable () const;

                /**
                 * @returns: the number of blocks sent persisted
                 */
                uint32_t getNbStored () const;

                /**
                 * @returns: the minimum number of blocks that must be stored on disk (if loaded is full)
                 */
                uint32_t getMinNbStorable () const;

                /**
                 * @returns: the number of allocated blocks
                 */
                uint32_t getNbAllocated () const;

                /**
                 * @returns: the number of uniq block loads since last counter reset
                 */
                uint32_t getUniqLoaded () const;

                /**
                 * ============================================================================
                 * ============================================================================
                 * ============================      SETTERS      =============================
                 * ============================================================================
                 * ============================================================================
                 * */

                /**
                 * Change the number of blocks that can be loaded at the same time
                 * @info: can be done dynamically (with already allocated blocks)
                 */
                void resize (uint32_t nbBlocks);

                /**
                 * Reset the uniq block loading counter
                 */
                void resetUniqCounter ();


                /**
                 * ============================================================================
                 * ============================================================================
                 * =============================      MISCS      ==============================
                 * ============================================================================
                 * ============================================================================
                 * */

                /**
                 * Dump the allocator debugging informations to the stream
                 */
                friend std::ostream & operator<< (std::ostream & s, rd_utils::memory::cache::Allocator & alloc);

                /**
                 * Debugging information about allocated blocks currently loaded into memory
                 */
                void printLoaded () const;

                /**
                 * Debugging information about allocated blocks
                 */
                void printBlocks () const;

        private:

                /**
                 * Load a block into memory
                 * @params:
                 *    - addr: the address of the block to load
                 */
                free_list_instance * load (uint32_t addr);

                /**
                 * Allocate a new block (and load it)
                 */
                uint8_t * allocateNewBlock (uint32_t & addr);

                /**
                 * Free a block from memory
                 */
                void freeBlock (uint32_t);

                /**
                 * Evict a number of blocks from the loaded blocks
                 */
                void evictSome (uint32_t nb);


        };


}
