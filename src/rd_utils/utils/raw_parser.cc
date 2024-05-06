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
    auto id = stream.receiveChar ();
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
    if (nb > 1024) throw std::runtime_error ("Too much element sent");

    char buffer [255];
    for (uint32_t i = 0 ; i < nb; i++) {
      auto n = stream.receiveU32 ();
      if (n > 255) throw std::runtime_error ("Key too big");
      stream.receive (buffer, n);
      buffer [n] = 0;

      auto value = this-> parse (stream);
      dict-> insert (buffer, value);
    }

    return dict;
  }


  std::shared_ptr <config::ConfigNode> RawParser::parseArray (net::TcpStream & stream) {
    auto array = std::make_shared <config::Array> ();
    uint32_t nb = stream.receiveU32 ();
    if (nb > 1024) throw std::runtime_error ("Too much element sent");

    for (uint32_t i = 0 ; i < nb; i++) {
      auto value = this-> parse (stream);
      array-> insert (value);
    }

    return array;
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseInt (net::TcpStream & stream) {
    int64_t value = stream.receiveI64 ();
    return std::make_shared <config::Int> (value);
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseFloat (net::TcpStream & stream) {
    double x;
    stream.receive ((char*) &x, sizeof (x));
    return std::make_shared <config::Float> (x);
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseString (net::TcpStream & stream) {
    uint32_t n = stream.receiveU32 ();
    if (n > 1024 * 1024) throw std::runtime_error ("String to long");
    if (n == 0) return std::make_shared <config::String> ("");

    char * buffer = new char[n + 1];
    stream.receive (buffer, n);
    buffer [n] = 0;

    auto ret = std::make_shared <config::String> (std::string (buffer));
    delete [] buffer;

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
    stream.sendChar ((uint8_t) RawParser::Types::DICT);
    stream.sendU32 (d.getKeys ().size ());
    for (auto & key : d.getValues ()) {
      stream.sendU32 (key.first.length ());
      stream.send (key.first.c_str (), key.first.length ());

      this-> dump (stream, *key.second);
    }
  }

  void RawParser::dumpArray (net::TcpStream & stream, const config::Array & d) {
    stream.sendChar ((uint8_t) RawParser::Types::ARRAY);
    stream.sendU32 (d.getLen ());
    for (uint32_t i = 0 ; i < d.getLen () ; i++) {
      this-> dump (stream, d [i]);
    }
  }

  void RawParser::dumpInt (net::TcpStream & stream, const config::Int & i) {
    stream.sendChar ((uint8_t) RawParser::Types::INT);
    stream.sendI64 (i.getI ());
  }

  void RawParser::dumpFloat (net::TcpStream & stream, const config::Float & f) {
    stream.sendChar ((uint8_t) RawParser::Types::FLOAT);
    double z = f.getF ();
    stream.send ((char*) &z, sizeof (double));
  }

  void RawParser::dumpString (net::TcpStream & stream, const config::String & s) {
    stream.sendChar ((uint8_t) RawParser::Types::STRING);
    stream.sendU32 (s.getStr ().length ());
    if (s.getStr ().length () != 0) {
      stream.send (s.getStr ().c_str (), s.getStr ().length ());
    }
  }

  void RawParser::dumpBool (net::TcpStream & stream, const config::Bool & b) {
    if (b.isTrue ()) {
      stream.sendChar ((uint8_t) RawParser::Types::BOOL_T);
    } else {
      stream.sendChar ((uint8_t) RawParser::Types::BOOL_F);
    }
  }



}
