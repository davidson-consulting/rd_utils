#pragma once

#include <cstdint>

namespace rd_utils::utils {

  struct MemorySize {
  private:

    // The size in bytes
    uint64_t _size;
    MemorySize (uint64_t);

  public:

    /**
     * Create a memory size from Bytes
     */
    static MemorySize B (uint64_t nb);

    /**
     * Create a memory size from KiloBytes
     */
    static MemorySize KB (uint64_t nb);

    /**
     * Create a memory size from MegaBytes
     */
    static MemorySize MB (uint64_t nb);

    /**
     * Create a memory size from GigaBytes
     */
    static MemorySize GB (uint64_t nb);

  public:

    /**
     * @returns: the size in bytes
     */
    uint64_t bytes () const;

    /**
     * @returns: the size in kilo bytes
     */
    uint64_t kilobytes () const;

    /**
     * @returns: the size in mega bytes
     */
    uint64_t megabytes () const;

    /**
     * @returns: the size in giga bytes
     */
    uint64_t gigabytes () const;

  };


}
