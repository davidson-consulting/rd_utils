#include <rd_utils/utils/error.hh>

namespace rd_utils::utils {

  Rd_UtilsError::Rd_UtilsError(const std::string& msg) : _msg(msg)
  {}

  const std::string & Rd_UtilsError::getMessage() const {
    return this->_msg;
  }

  Rd_UtilsError::~Rd_UtilsError () {}
			

  addr_error::addr_error (const std::string & msg) : Rd_UtilsError (msg)
  {}
}


