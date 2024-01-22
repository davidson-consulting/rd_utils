#pragma once

#include "base.hh"
#include <string>

namespace rd_utils::utils::config {

  class String : public ConfigNode {

    std::string _s;

  public:

    String (const std::string&);

    /**
     * @returns: the value contains in the float
     */
    const std::string & getStr () const override;

    void format (std::ostream & ) const override;

    ~String ();
  };



}
