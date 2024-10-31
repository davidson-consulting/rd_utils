#pragma once

#include <cstdint>
#include <iostream>
#include <string>

namespace rd_utils::utils {

  struct MemorySize {
  public:

    enum class Unit : uint8_t {
      B = 1,
      KB,
      MB,
      GB
    };

  private:

    // The size in bytes
    uint64_t _size;
    MemorySize (uint64_t);

  public:

    static MemorySize unit (uint64_t nb, const std::string & unit);

    static MemorySize unit (uint64_t nb, MemorySize::Unit unit);

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


    MemorySize operator+ (MemorySize other) const;
    MemorySize operator- (MemorySize other) const;
    MemorySize operator* (uint64_t cst) const;
    MemorySize operator/ (uint64_t cst) const;

    bool operator> (MemorySize other) const;
    bool operator< (MemorySize other) const;
    bool operator== (MemorySize other) const;
    bool operator!= (MemorySize other) const;

    void operator-= (MemorySize other);
    void operator+= (MemorySize other);

    static MemorySize min (MemorySize A, MemorySize B);
    static MemorySize max (MemorySize A, MemorySize B);
  };


}

std::ostream & operator<< (std::ostream & o, rd_utils::utils::MemorySize m);
