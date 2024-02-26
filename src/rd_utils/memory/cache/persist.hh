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

  public:

    /**
     * @params:
     *    - path: the directory in which block will be persisted
     */
    BlockPersister (const std::string & path = "/tmp/");

    /**
     * Load a block from disk
     * @warning: delete the block from disk
     */
    void load (uint64_t addr, uint8_t* memory, uint64_t size);

    /**
     * Save a block to disk
     */
    void save (uint64_t addr, uint8_t* memory, uint64_t size);

    /**
     * Delete a block from disk
     */
    void erase (uint64_t addr);

  };


}
