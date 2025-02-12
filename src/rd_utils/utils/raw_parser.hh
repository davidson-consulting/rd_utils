#pragma once

#include <rd_utils/utils/config/_.hh>
#include <rd_utils/net/stream.hh>
#include <rd_utils/utils/lexer.hh>
#include <cstdint>

namespace rd_utils::utils::raw {

  class RawParser {
  private:

    enum class Types : uint8_t {
      DICT = 1,
      ARRAY,
      INT,
      STRING,
      FLOAT,
      BOOL_T,
      BOOL_F
    };

  public:

    RawParser ();

    /**
     * parse a raw config string
     */
    std::shared_ptr<config::ConfigNode> parse (uint8_t *& buffer, uint32_t & len);

    void dump (std::vector <uint8_t> & buffer, const config::ConfigNode & node);

  private:

    std::shared_ptr<config::ConfigNode> parseValue (uint8_t*&, uint32_t&);
    std::shared_ptr<config::ConfigNode> parseDict (uint8_t*&, uint32_t&);
    std::shared_ptr<config::ConfigNode> parseArray (uint8_t*&, uint32_t&);
    std::shared_ptr<config::ConfigNode> parseInt (uint8_t*&, uint32_t&);
    std::shared_ptr<config::ConfigNode> parseFloat (uint8_t*&, uint32_t&);
    std::shared_ptr<config::ConfigNode> parseString (uint8_t*&, uint32_t&);

  private:

    void dumpDict (std::vector <uint8_t> &, const config::Dict & d);
    void dumpArray (std::vector <uint8_t> &, const config::Array & d);
    void dumpInt (std::vector <uint8_t> &, const config::Int & d);
    void dumpFloat (std::vector <uint8_t> &, const config::Float & d);
    void dumpString (std::vector <uint8_t> &, const config::String & d);
    void dumpBool (std::vector <uint8_t> &, const config::Bool & d);


  };

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * =============================          DESERIALIZATION UTILS          ==============================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint8_t readU8 (uint8_t*&buffer, uint32_t &len);
  uint32_t readU32 (uint8_t*&buffer, uint32_t &len);
  uint64_t readU64 (uint8_t*&buffer, uint32_t &len);
  int64_t readI64 (uint8_t*&buffer, uint32_t &len);
  double readF64 (uint8_t*&buffer, uint32_t &len);
  std::string readString (uint8_t*&buffer, uint32_t &len, uint32_t nb);

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ==============================          SERIALIZATION UTILS          ===============================
   * ====================================================================================================
   * ====================================================================================================
   */

  void writeU8 (std::vector <uint8_t> & buffer, uint8_t value);
  void writeU32 (std::vector <uint8_t> & buffer, uint32_t value);
  void writeU64 (std::vector <uint8_t> & buffer, uint64_t);
  void writeI64 (std::vector <uint8_t> & buffer, int64_t);
  void writeF64 (std::vector <uint8_t> & buffer, double);
  void writeString (std::vector <uint8_t> & buffer, const std::string & v);

}
