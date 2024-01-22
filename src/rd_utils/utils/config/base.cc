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

  int32_t ConfigNode::getOr (const std::string & key, int32_t value) const {
    try {
      std::cout << "I ???" << std::endl;
      return (*this) [key].getI ();
    } catch (...) {
      return value;
    }
  }

  double ConfigNode::getOr (const std::string & key, double value) const {
    try {
      std::cout << "F ???" << std::endl;
      return (*this) [key].getF ();
    } catch (...) {
      return value;
    }
  }

  std::string ConfigNode::getOr (const std::string & key, const char* value) const {
    try {
      std::cout << "Str ???" << std::endl;
      return (*this) [key].getStr ();
    } catch (...) {
      std::cout << "?" << std::endl;
      return std::string (value);
    }
  }


  std::string ConfigNode::getOr (const std::string & key, std::string value) const {
    try {
      std::cout << "Str ???" << std::endl;
      return (*this) [key].getStr ();
    } catch (...) {
      std::cout << "?" << std::endl;
      return value;
    }
  }

  bool ConfigNode::getOr (const std::string & key, bool value) const {
    try {
      std::cout << "B ???" << std::endl;
      return (*this) [key].isTrue ();
    } catch (...) {
      return value;
    }
  }

  ConfigNode::~ConfigNode () {}

}


std::ostream & operator<< (std::ostream & out, const rd_utils::utils::config::ConfigNode & node) {
  node.format (out);
  return out;
}
