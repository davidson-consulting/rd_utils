#include "array.hh"
#include "int.hh"
#include "bool.hh"
#include "str.hh"
#include "float.hh"

namespace rd_utils::utils::config {

  const ConfigNode & Array::operator[] (uint32_t i) const {
    if (this-> _nodes.size () <= i) throw ConfigError ();
    return *this-> _nodes [i];
  }

  const std::shared_ptr <ConfigNode> Array::get (uint32_t i) const {
    if (this-> _nodes.size () <= i) throw ConfigError ();
    return this-> _nodes [i];
  }

  uint32_t Array::getLen () const {
    return this-> _nodes.size ();
  }

  Array& Array::insert (const std::string & value) {
    return this-> insert (std::make_shared <config::String> (value));
  }

  Array& Array::insert (int64_t value) {
    return this-> insert (std::make_shared <config::Int> (value));
  }

  Array& Array::insert (double value) {
    return this-> insert (std::make_shared <config::Float> (value));
  }

  Array& Array::insert (std::shared_ptr<ConfigNode> node) {
    this-> _nodes.push_back (node);
    return *this;
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
