#include "dict.hh"
#include "int.hh"
#include "bool.hh"
#include "str.hh"
#include "float.hh"

namespace rd_utils::utils::config {

  const ConfigNode& Dict::operator[] (const std::string & key) const {
    auto it = this-> _nodes.find (key);
    if (it == this-> _nodes.end ()) throw ConfigError ();

    return *it-> second;
  }

  bool Dict::contains (const std::string & key) const {
    auto it = this-> _nodes.find (key);
    return (it != this-> _nodes.end ());
  }


  const std::set <std::string> & Dict::getKeys () const {
    return this-> _keys;
  }

  const std::map <std::string, std::shared_ptr <ConfigNode> > & Dict::getValues () const {
    return this-> _nodes;
  }


  Dict& Dict::insert (const std::string & key, const std::string & value) {
    return this-> insert (key, std::make_shared <config::String> (value));
  }

  Dict& Dict::insert (const std::string & key, int64_t value) {
    return this-> insert (key, std::make_shared <config::Int> (value));
  }

  Dict& Dict::insert (const std::string & key, double value) {
    return this-> insert (key, std::make_shared <config::Float> (value));
  }

  Dict& Dict::insert (const std::string & key, std::shared_ptr<ConfigNode> value) {
    this-> _keys.insert (key);
    this-> _nodes.insert_or_assign (key, value);
    return *this;
  }

  std::shared_ptr<Dict> Dict::getInDic (const std::string & key) {
    auto it = this-> _nodes.find (key);
    if (it != this-> _nodes.end ()) {
      return std::dynamic_pointer_cast <Dict> (it-> second);
    }

    return nullptr;
  }

  void Dict::format (std::ostream & s) const {
    s << "{";
    uint32_t i = 0;
    for (auto & [k, v] : this-> _nodes) {
      if (i != 0) s << ", ";
      s << k << " => ";
      v-> format (s);
      i += 1;
    }
    s << "}";
  }

  Dict::~Dict () {
    this-> _nodes.clear ();
  }

}
