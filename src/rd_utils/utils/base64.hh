#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace rd_utils::utils {

  /**
   * Encode a byte array of chars into a readable string
   * */
  std::string base64_encode(uint8_t const* buf, unsigned int bufLen, bool withSpec = true);

  /**
   * Decode a string into an array of bytes
   */
  std::vector<uint8_t> base64_decode(std::string const&);

}
