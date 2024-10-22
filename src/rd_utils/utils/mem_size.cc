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

  MemorySize MemorySize::operator+ (MemorySize other) const {
    return MemorySize (this-> _size + other._size);
  }

  MemorySize MemorySize::operator- (MemorySize other) const {
    return MemorySize (this-> _size - other._size);
  }

  MemorySize MemorySize::operator* (uint64_t cst) const {
    return MemorySize (this-> _size * cst);
  }

  MemorySize MemorySize::operator/ (uint64_t cst) const {
    return MemorySize (this-> _size / cst);
  }

  MemorySize MemorySize::min (MemorySize A, MemorySize B) {
    return MemorySize (A._size > B._size ? B._size : A._size);
  }

  MemorySize MemorySize::max (MemorySize A, MemorySize B) {
    return MemorySize (A._size > B._size ? A._size : B._size);
  }

}


std::ostream & operator<< (std::ostream & o, rd_utils::utils::MemorySize m) {
  o << "M (" << m.bytes () << "B)";
  return o;
}
