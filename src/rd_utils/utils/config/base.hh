#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <memory>
#include "../error.hh"


namespace rd_utils::utils::config {

  class ConfigNode {
  public:

    /**
     * Access a subg config at index 'i'
     * @throws:
     *    - config_error: if not defined
     */
    virtual const ConfigNode& operator[] (uint32_t i) const;

    /**
     * Access a subg config at index 'key'
     * @throws:
     *    - config_error: if not defined
     */
    virtual const ConfigNode& operator[](const std::string & key) const;

    /**
     * @returns: true if the value can be evaluated as true
     */
    virtual bool isTrue () const;

    /**
     * @returns: a int value if the value can be evaluated as an int value
     */
    virtual int32_t getI () const;

    /**
     * @returns: a int value if the value can be evaluated as a float value
     */
    virtual double getF () const;

    /**
     * @returns: a int value if the value can be evaluated as a string value
     */
    virtual const std::string & getStr () const;

    /**
     * @returns: true if the node contains the key 'key'
     */
    virtual bool contains (const std::string & key) const;;

    /**
     * Format the config node into a printable string
     */
    virtual void format (std::ostream & s) const = 0;


    virtual ~ConfigNode ();

  };


}

std::ostream & operator<< (std::ostream & out, const rd_utils::utils::config::ConfigNode & node);
