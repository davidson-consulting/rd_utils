#pragma once

#include <string>
#include <cstdio>

namespace rd_utils::memory::cache {


  /**
   * Class to read/write blocks from memory
   */
  class BlockPersister {
  private:

    // The root path of the persister
    std::string _path;

    // The number of time a block had to be loaded
    uint32_t _nbLoaded;

    // The number of times a block was saved to disk
    uint32_t _nbSaved;

  public:

    /**
     * @params:
     *    - path: the directory in which block will be persisted
     */
    BlockPersister (const std::string & path = "./");

    /**
     * Load a block from disk
     * @warning: delete the block from disk
     */
    bool load (uint64_t addr, uint8_t* memory, uint64_t size);

    /**
     * Save a block to disk
     */
    void save (uint64_t addr, uint8_t* memory, uint64_t size);

    /**
     * Delete a block from disk
     */
    void erase (uint64_t addr);

    void printInfo () const;

  };


}
