#pragma once

#include <iostream>

namespace rd_utils::utils {

  class Rd_UtilsError {

    // The error message of the error
    std::string _msg;

  public:

    /**
     * Create an error with a specific message
     */
    Rd_UtilsError(const std::string& msg);

    /**
     * @returns: the error message of the error
     */
    const std::string& getMessage() const;

    virtual ~Rd_UtilsError();

  };

  class addr_error : Rd_UtilsError {
  public:
    addr_error (const std::string & msg);
  };

}
