#pragma once

#include "base.hh"


namespace rd_utils::utils::config {

  class Float : public ConfigNode {

    double _f;

  public:

    Float (double);

    /**
     * @returns: the value contains in the float
     */
    double getF () const override;

    /**
     * @returns: the value contains in the float casted into an int
     */
    int32_t getI () const override;

    void format (std::ostream & ) const override;

    ~Float ();
  };



}
