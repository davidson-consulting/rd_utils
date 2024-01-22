#include "array.hh"

namespace rd_utils::utils::config {

  const ConfigNode & Array::operator[] (uint32_t i) const {
    if (this-> _nodes.size () <= i) throw ConfigError ();
    return *this-> _nodes [i];
  }

  void Array::insert (std::shared_ptr<ConfigNode> node) {
    this-> _nodes.push_back (node);
  }

  void Array::format (std::ostream & s) const {
    s << '[';
    for (uint32_t i = 0 ; i < this-> _nodes.size (); i++) {
      if (i != 0) s << ", ";
      this-> _nodes [i]-> format (s);
    }
    s << ']';
  }

  Array::~Array () {
    this-> _nodes.clear ();
  }


}
