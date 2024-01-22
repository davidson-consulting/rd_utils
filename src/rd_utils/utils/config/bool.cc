#include "bool.hh"


namespace rd_utils::utils::config {

  Bool::Bool (bool b)
    : _b (b)
  {}

  bool Bool::isTrue () const {
    return this-> _b;
  }

  void Bool::format (std::ostream & s) const {
    if (this-> _b) {
      s << "true";
    } else s << "false";
  }

  Bool::~Bool () {}

}
