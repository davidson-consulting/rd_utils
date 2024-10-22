#include "mem_size.hh"

namespace rd_utils::utils {

  MemorySize::MemorySize (uint64_t nb)
    :_size (nb)
  {}

  MemorySize MemorySize::B (uint64_t nb) {
    return MemorySize (nb);
  }

  MemorySize MemorySize::KB (uint64_t nb) {
    return MemorySize (nb * 1024);
  }

  MemorySize MemorySize::MB (uint64_t nb) {
    return MemorySize (nb * 1024 * 1024);
  }

  MemorySize MemorySize::GB (uint64_t nb) {
    return MemorySize (nb * 1024 * 1024 * 1024);
  }

  uint64_t MemorySize::bytes () const {
    return this-> _size;
  }

  uint64_t MemorySize::kilobytes () const {
    return this-> _size / 1024;
  }

  uint64_t MemorySize::megabytes () const {
    return this-> _size / 1024 / 1024;
  }

  uint64_t MemorySize::gigabytes () const {
    return this-> _size / 1024 / 1024 / 1024;
  }

}
