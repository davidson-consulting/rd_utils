#pragma once

#include <iostream>
#include <cstdint>

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

  class AddrError : Rd_UtilsError {
  public:
    AddrError (const std::string & msg);
  };


  struct LexerError : Rd_UtilsError {

    uint32_t line;

    LexerError(uint32_t line, const std::string& msg);

  };

  struct ConfigError : Rd_UtilsError {
    ConfigError ();
  };

  class FileError : public utils::Rd_UtilsError {
  public:
    FileError(const std::string& msg);
  };


}
