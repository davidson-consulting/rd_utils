#pragma once

#include "base.hh"
#include <vector>

namespace rd_utils::utils::config {

  class Array : public ConfigNode {
  private:

    // The content of the array
    std::vector <std::shared_ptr<ConfigNode> > _nodes;

  public:

    /**
     * @returns: the node at index 'i'
     * @throws:
     *    - config_error: if array len <= i
     */
    const ConfigNode& operator[] (uint32_t i) const override;

    /**
     * Insert a node in the array
     */
    void insert (std::shared_ptr<ConfigNode> node);

    /**
     * Transform the node into a printable string
     */
    void format (std::ostream & s) const override;

    ~Array ();

  };




}
