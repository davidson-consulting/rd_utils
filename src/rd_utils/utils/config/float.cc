#include "float.hh"

namespace rd_utils::utils::config {

  Float::Float (double f) : _f (f)
  {}

  double Float::getF () const {
    return this-> _f;
  }

  int64_t Float::getI () const {
    return (int64_t) this-> _f;
  }

  void Float::format (std::ostream & s) const {
    s << this-> _f;
  }

  Float::~Float () {}

}
