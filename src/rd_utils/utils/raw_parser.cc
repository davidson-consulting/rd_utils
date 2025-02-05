#include "raw_parser.hh"


namespace rd_utils::utils::raw {


  std::shared_ptr<config::ConfigNode> parse (net::TcpStream & stream) {
    RawParser p;
    return p.parse (stream);
  }

  void dump (net::TcpStream & stream, const config::ConfigNode & node) {
    RawParser p;
    p.dump (stream, node);
  }

  RawParser::RawParser () {}

  std::shared_ptr<config::ConfigNode> RawParser::parse (net::TcpStream & stream) {
    uint8_t id = stream.receiveChar ();
    switch ((RawParser::Types) id) {
    case Types::DICT:
      return this-> parseDict (stream);
    case Types::ARRAY:
      return this-> parseArray (stream);
    case Types::INT:
      return this-> parseInt (stream);
    case Types::STRING:
      return this-> parseString (stream);
    case Types::FLOAT:
      return this-> parseFloat (stream);
    case Types::BOOL_T:
      return std::make_shared <config::Bool> (true);
    case Types::BOOL_F:
      return std::make_shared <config::Bool> (false);
    default:
      throw std::runtime_error ("Failed to parse from raw stream");
    }
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseDict (net::TcpStream & stream) {
    auto dict = std::make_shared <config::Dict> ();
    uint32_t nb = stream.receiveU32 ();
    if (nb > 2048) throw std::runtime_error ("Too much element sent");

    for (uint32_t i = 0 ; i < nb; i++) {
      auto n = stream.receiveU32 ();
      auto key = stream.receiveStr (n);

      auto value = this-> parse (stream);
      dict-> insert (key, value);
    }

    return dict;
  }


  std::shared_ptr <config::ConfigNode> RawParser::parseArray (net::TcpStream & stream) {
    auto array = std::make_shared <config::Array> ();
    auto nb = stream.receiveU32 ();
    if (nb > 2048) throw std::runtime_error ("Too much element sent");

    for (uint32_t i = 0 ; i < nb; i++) {
      auto value = this-> parse (stream);
      array-> insert (value);
    }

    return array;
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseInt (net::TcpStream & stream) {
    auto value = stream.receiveI64 ();
    return std::make_shared <config::Int> (value);
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseFloat (net::TcpStream & stream) {
    auto x = stream.receiveF64 ();
    return std::make_shared <config::Float> (x);
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseString (net::TcpStream & stream) {
    auto n = stream.receiveU32 ();
    if (n > 1024 * 1024) throw std::runtime_error ("String to long");
    if (n == 0) return std::make_shared <config::String> ("");

    auto str = stream.receiveStr (n);
    auto ret = std::make_shared <config::String> (str);
    return ret;
  }

  void RawParser::dump (net::TcpStream & stream, const config::ConfigNode & node) {
    match (node) {
      of (config::Dict, n) {
        this-> dumpDict (stream, *n);
      } elof (config::Array, a) {
        this-> dumpArray (stream, *a);
      } elof (config::Int, i) {
        this-> dumpInt (stream, *i);
      } elof (config::Float, f) {
        this-> dumpFloat (stream, *f);
      } elof (config::String, s) {
        this-> dumpString (stream, *s);
      } elof (config::Bool, b) {
        this-> dumpBool (stream, *b);
      } elfo {
        throw std::runtime_error ("Unknown config type");
      }
    }
  }

  void RawParser::dumpDict (net::TcpStream & stream, const config::Dict & d) {
    stream.sendChar ((uint8_t) RawParser::Types::DICT, true);
    stream.sendU32 (d.getKeys ().size (), true);
    for (auto & key : d.getValues ()) {
      stream.sendU32 (key.first.length (), true);
      stream.sendStr (key.first, true);

      this-> dump (stream, *key.second);
    }
  }

  void RawParser::dumpArray (net::TcpStream & stream, const config::Array & d) {
    stream.sendChar ((uint8_t) RawParser::Types::ARRAY, true);
    stream.sendU32 (d.getLen (), true);
    for (uint32_t i = 0 ; i < d.getLen () ; i++) {
      this-> dump (stream, d [i]);
    }
  }

  void RawParser::dumpInt (net::TcpStream & stream, const config::Int & i) {
    stream.sendChar ((uint8_t) RawParser::Types::INT, true);
    stream.sendI64 (i.getI (), true);
  }

  void RawParser::dumpFloat (net::TcpStream & stream, const config::Float & f) {
    stream.sendChar ((uint8_t) RawParser::Types::FLOAT, true);
    double z = f.getF ();
    stream.sendF64 (z, true);
  }

  void RawParser::dumpString (net::TcpStream & stream, const config::String & s) {
    stream.sendChar ((uint8_t) RawParser::Types::STRING, true);
    stream.sendU32 (s.getStr ().length (), true);
    if (s.getStr ().length () != 0) {
      stream.sendStr (s.getStr (), true);
    }
  }

  void RawParser::dumpBool (net::TcpStream & stream, const config::Bool & b) {
    if (b.isTrue ()) {
      stream.sendChar ((uint8_t) RawParser::Types::BOOL_T, true);
    } else {
      stream.sendChar ((uint8_t) RawParser::Types::BOOL_F, true);
    }
  }



}
