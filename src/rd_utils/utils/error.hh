#pragma once

#include <iostream>
#include <cstdint>
#include <stdexcept>

namespace rd_utils::utils {

  class Rd_UtilsError : public std::runtime_error {
  public:

    /**
     * Create an error with a specific message
     */
    Rd_UtilsError(const std::string& msg);

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
