#include "str.hh"

namespace rd_utils::utils::config {

  String::String (const std::string & s) : _s (s)
  {}

  const std::string & String::getStr () const {
    return this-> _s;
  }

  void String::format (std::ostream & s) const {
    s << this-> _s;
  }

  String::~String () {}

}
