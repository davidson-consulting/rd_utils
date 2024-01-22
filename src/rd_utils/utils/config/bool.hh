#pragma once

#include "base.hh"


namespace rd_utils::utils::config {

  class Bool : public ConfigNode {

    bool _b;

  public:

    Bool (bool);

    /**
     * @returns: true iif the value contains in the bool is true
     */
    bool isTrue () const override;

    void format (std::ostream & ) const override;

    ~Bool ();
  };



}
