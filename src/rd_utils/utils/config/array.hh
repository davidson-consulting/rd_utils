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
     * @returns: the number of elements in the array
     */
    uint32_t getLen () const;

    /**
     * Insert a node in the array
     */
    Array& insert (std::shared_ptr<ConfigNode> node);
    Array& insert (const std::string & str);
    Array& insert (int64_t);
    Array& insert (double);

    /**
     * Transform the node into a printable string
     */
    void format (std::ostream & s) const override;

    ~Array ();

  };




}
