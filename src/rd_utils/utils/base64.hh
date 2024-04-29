#pragma once

#include <vector>
#include <string>

namespace rd_utils::utils {

  typedef unsigned char BYTE;

  /**
   * Encode a byte array of chars into a readable string
   * */
  std::string base64_encode(BYTE const* buf, unsigned int bufLen, bool withSpec = true);

  /**
   * Decode a string into an array of bytes
   */
  std::vector<BYTE> base64_decode(std::string const&);

}
