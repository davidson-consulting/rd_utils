#include "int.hh"

namespace rd_utils::utils::config {

  Int::Int (int64_t i) : _i (i)
  {}

  int64_t Int::getI () const {
    return this-> _i;
  }

  double Int::getF () const {
    return (double) this-> _i;
  }

  void Int::format (std::ostream & s) const {
    s << this-> _i;
  }

  Int::~Int () {}

}
