#include "mem_size.hh"
#include <cctype>

namespace rd_utils::utils {
  
  MemorySize::MemorySize(): _size(0) {}

  MemorySize::MemorySize (uint64_t nb)
    :_size (nb)
  {}

  MemorySize MemorySize::str (const std::string & value) {
    if (value.length () > 2) {
      auto unit = value.substr (value.length () - 2);
      return MemorySize::unit (std::strtoull (value.substr (0, value.length () - 2).c_str (), nullptr, 0), unit);
    }

    throw std::runtime_error ("malformed memory size");
  }

  MemorySize MemorySize::unit (uint64_t nb, const std::string & unit) {
    if (unit.length () == 1 && unit [0] == 'b') return MemorySize::B (nb);
    if (unit.length () != 2) throw std::runtime_error ("unknown unit");
    if (tolower (unit [1]) != 'b') throw std::runtime_error ("unknown unit");

    switch (tolower (unit [0])) {
    case 'k' :
      return MemorySize::KB (nb);
    case 'm':
      return MemorySize::MB (nb);
    case 'g':
      return MemorySize::GB (nb);
    default:
      throw std::runtime_error ("unknown unit");
    }
  }


  MemorySize MemorySize::unit (uint64_t nb, MemorySize::Unit unit) {
    switch (unit) {
    case MemorySize::Unit::B :
      return MemorySize::B (nb);
    case MemorySize::Unit::KB :
      return MemorySize::KB (nb);
    case MemorySize::Unit::MB :
      return MemorySize::MB (nb);
    case MemorySize::Unit::GB :
      return MemorySize::GB (nb);
    default:
      throw std::runtime_error ("unknown unit");
    }
  }

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

  MemorySize MemorySize::operator* (MemorySize other) const {
    return this->bytes() * other.bytes();
  }

  MemorySize MemorySize::operator/ (MemorySize other) const {
    return this->bytes() / other.bytes();
  }

  MemorySize MemorySize::operator% (MemorySize other) const {
    return this->bytes() % other.bytes();
  }

  MemorySize MemorySize::operator* (uint64_t cst) const {
    return MemorySize (this-> _size * cst);
  }

  MemorySize MemorySize::operator/ (uint64_t cst) const {
    return MemorySize (this-> _size / cst);
  }

  bool MemorySize::operator> (MemorySize other) const {
    return this->bytes() > other.bytes();
  }

  bool MemorySize::operator< (MemorySize other) const {
    return this->bytes() < other.bytes();
  }

  bool MemorySize::operator== (MemorySize other) const {
    return this->bytes() == other.bytes();
  }
  
  bool MemorySize::operator!= (MemorySize other) const {
    return this->bytes() != other.bytes();
  } 

  void MemorySize::operator-= (MemorySize other) {
    this->_size -= other._size;
  }

  void MemorySize::operator+= (MemorySize other) {
    this->_size += other._size;
  }

  MemorySize MemorySize::min (MemorySize A, MemorySize B) {
    return MemorySize (A._size > B._size ? B._size : A._size);
  }

  MemorySize MemorySize::max (MemorySize A, MemorySize B) {
    return MemorySize (A._size > B._size ? A._size : B._size);
  }

  MemorySize MemorySize::nextPow2 (MemorySize A) {
    if (A.bytes () == 1) return A;
    else {
      return MemorySize::B (1 << (64 - __builtin_clzl (A.bytes () - 1)));
    }
  }

  MemorySize MemorySize::roundUp (MemorySize A, MemorySize rnd) {
    if (rnd.bytes () == 0) {
      return A;
    }

    uint64_t remainder = A.bytes () % rnd.bytes ();
    if (remainder == 0) {
      return A;
    }

    return A + (rnd - remainder);
  }

}


std::ostream & operator<< (std::ostream & o, const rd_utils::utils::MemorySize & m) {
  o << "M (" << m.megabytes () << "MB)";
  return o;
}
