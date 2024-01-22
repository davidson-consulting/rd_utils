#include "dict.hh"


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

  void Dict::insert (const std::string & key, std::shared_ptr<ConfigNode> value) {
    this-> _keys.insert (key);
    this-> _nodes.emplace (key, value);
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
