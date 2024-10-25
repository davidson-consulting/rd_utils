#include <rd_utils/utils/error.hh>

namespace rd_utils::utils {

  Rd_UtilsError::Rd_UtilsError(const std::string& msg)
    : std::runtime_error (msg)
  {}


  AddrError::AddrError (const std::string & msg)
    : Rd_UtilsError (msg)
  {}

  LexerError::LexerError (uint32_t line, const std::string & msg)
    : Rd_UtilsError (msg)
    , line (line)
  {}

  ConfigError::ConfigError () :
    Rd_UtilsError ("")
  {}

}
