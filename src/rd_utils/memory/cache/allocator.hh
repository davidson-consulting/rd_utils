#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>

#include "free_list.hh"
#include "persist.hh"
#include <rd_utils/concurrency/mutex.hh>

namespace rd_utils::memory::cache {

    struct AllocatedSegment {
        uint32_t blockAddr;
        uint32_t offset;
    };

    struct BlockInfo {
        uint32_t lru;
        uint32_t max_size;
    };

    class Allocator {
    private:

        // The maximum number of blocks that can be loaded at the same time
        uint32_t _max_blocks;

        // The size of a block
        uint32_t _block_size;

        // The list of block currently in memory (id-> memory)
        std::unordered_map <uint32_t, uint8_t*> _loaded;

        // The list of blocks allocated by the allocator (id -> lru)
        std::unordered_map <uint32_t, BlockInfo> _blocks;

        // The persister to store blocks to disk
        BlockPersister _perister;

        // The last block addr
        uint32_t _last_block;

    private:

        Allocator (const Allocator&);

        void operator=(const Allocator&);

        // The global allocator
        static Allocator * __GLOBAL__;

        static concurrency::mutex __GLOBAL_MUTEX__;

    public:

        /**
         * @params:
         *    - totalSize: the maximum available size of the local memory
         *    - blockSize: the size of a block (defining the number of block that can be loaded at the same time)
         */
        Allocator (uint64_t totalSize, uint32_t blockSize = 1024 * 1024);

        /**
         * Allocate a segment of memory
         * @returns:
         *    - true iif a segment was found
         *    - alloc: the allocated segment
         */
        bool allocate (uint32_t size, AllocatedSegment & alloc);

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
         * Free an allocated memory segment
         */
        void free (AllocatedSegment alloc);

        /**
         * @returns: the global instance of the allocator
         */
        static Allocator* instance ();

    private:

        /**
         * Evict a number of blocks from the loaded blocks
         */
        void evictSome (uint32_t nb);

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

    };


}
