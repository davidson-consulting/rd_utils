#pragma once

#include <rd_utils/utils/config/_.hh>
#include <rd_utils/net/stream.hh>
#include <rd_utils/utils/lexer.hh>
#include <cstdint>

namespace rd_utils::utils::raw {

  /**
   * Parse a configuration in raw str
   */
  std::shared_ptr<config::ConfigNode> parse (net::TcpStream &);

  /**
   * Dump a configuration in raw
   */
  void dump (net::TcpStream &, const config::ConfigNode&);

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
    std::shared_ptr<config::ConfigNode> parse (net::TcpStream & stream);

    void dump (net::TcpStream& stream, const config::ConfigNode & node);


  private:

    std::shared_ptr<config::ConfigNode> parseValue (net::TcpStream &);

    std::shared_ptr<config::ConfigNode> parseDict (net::TcpStream &);

    std::shared_ptr<config::ConfigNode> parseArray (net::TcpStream &);

    std::shared_ptr<config::ConfigNode> parseInt (net::TcpStream &);

    std::shared_ptr<config::ConfigNode> parseFloat (net::TcpStream &);

    std::shared_ptr<config::ConfigNode> parseString (net::TcpStream &);

  private:

    void dumpDict (net::TcpStream &, const config::Dict & d);

    void dumpArray (net::TcpStream &, const config::Array & d);

    void dumpInt (net::TcpStream &, const config::Int & d);

    void dumpFloat (net::TcpStream &, const config::Float & d);

    void dumpString (net::TcpStream &, const config::String & d);

    void dumpBool (net::TcpStream &, const config::Bool & d);

  };

}
