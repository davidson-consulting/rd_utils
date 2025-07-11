#pragma once

#include "base.hh"


namespace rd_utils::utils::config {

  class Int : public ConfigNode {

    int64_t _i;

  public:

    Int (int64_t);

    /**
     * @returns: the value contains in the int
     */
    int64_t getI () const override;

    /**
     * @returns: the value contains in the int casted into a float
     */
    double getF () const override;

    void format (std::ostream & ) const override;

    ~Int ();
  };



}
