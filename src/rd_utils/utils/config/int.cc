#include "int.hh"

namespace rd_utils::utils::config {

  Int::Int (int32_t i) : _i (i)
  {}

  int32_t Int::getI () const {
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
