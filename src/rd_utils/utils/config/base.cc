#include "base.hh"

namespace rd_utils::utils::config {

  const ConfigNode& ConfigNode::operator[] (uint32_t) const {
    throw ConfigError ();
  }

  const ConfigNode& ConfigNode::operator[] (const std::string & key) const {
    throw ConfigError ();
  }

  bool ConfigNode::contains (const std::string & key) const {
    return false;
  }


  bool ConfigNode::isTrue () const {
    throw ConfigError ();
  }

  int32_t ConfigNode::getI () const {
    throw ConfigError ();
  }

  double ConfigNode::getF () const {
    throw ConfigError ();
  }

  const std::string & ConfigNode::getStr () const {
    throw ConfigError ();
  }

  ConfigNode::~ConfigNode () {}

}


std::ostream & operator<< (std::ostream & out, const rd_utils::utils::config::ConfigNode & node) {
  node.format (out);
  return out;
}
