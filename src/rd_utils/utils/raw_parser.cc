#include "raw_parser.hh"
#include "mem_size.hh"

namespace rd_utils::utils::raw {


  std::shared_ptr<config::ConfigNode> parse (net::TcpStream & stream) {
    RawParser p;
    std::vector <uint8_t> buffer;
    auto size = stream.receiveU32 ();
    if (size > MemorySize::MB (1).bytes ()) throw std::runtime_error ("message to big");
    buffer.resize (size);
    stream.receiveRaw (buffer.data (), size, true);

    uint8_t * data = buffer.data ();
    return p.parse (data, size);
  }

  void dump (net::TcpStream & stream, const config::ConfigNode & node) {
    RawParser p;
    std::vector <uint8_t> buffer;
    p.dump (buffer, node);

    stream.sendU32 (buffer.size ());
    if (buffer.size () != 0) {
      stream.sendRaw (buffer.data (), buffer.size (), true);
    }
  }

  RawParser::RawParser () {}

  std::shared_ptr<config::ConfigNode> RawParser::parse (uint8_t *& buffer, uint32_t &len) {
    uint8_t id = this-> readU8 (buffer, len);
    switch ((RawParser::Types) id) {
    case Types::DICT:
      return this-> parseDict (buffer, len);
    case Types::ARRAY:
      return this-> parseArray (buffer, len);
    case Types::INT:
      return this-> parseInt (buffer, len);
    case Types::STRING:
      return this-> parseString (buffer, len);
    case Types::FLOAT:
      return this-> parseFloat (buffer, len);
    case Types::BOOL_T:
      return std::make_shared <config::Bool> (true);
    case Types::BOOL_F:
      return std::make_shared <config::Bool> (false);
    default:
      throw std::runtime_error ("Failed to parse from raw stream");
    }
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseDict (uint8_t *& buffer, uint32_t &len) {
    auto dict = std::make_shared <config::Dict> ();
    uint32_t nb = this-> readU32 (buffer, len);
    if (nb > 2048) throw std::runtime_error ("Too much element sent");

    for (uint32_t i = 0 ; i < nb; i++) {
      auto n = this-> readU32 (buffer, len);
      auto key = this-> readString (buffer, len, n);

      auto value = this-> parse (buffer, len);
      dict-> insert (key, value);
    }

    return dict;
  }


  std::shared_ptr <config::ConfigNode> RawParser::parseArray (uint8_t *& buffer, uint32_t &len) {
    auto array = std::make_shared <config::Array> ();
    uint32_t nb = this-> readU32 (buffer, len);
    if (nb > 2048) throw std::runtime_error ("Too much element sent");

    for (uint32_t i = 0 ; i < nb; i++) {
      auto value = this-> parse (buffer, len);
      array-> insert (value);
    }

    return array;
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseInt (uint8_t *& buffer, uint32_t &len) {
    auto value = this-> readI64 (buffer, len);
    return std::make_shared <config::Int> (value);
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseFloat (uint8_t *& buffer, uint32_t &len) {
    auto value = this-> readF64 (buffer, len);
    return std::make_shared <config::Float> (value);
  }

  std::shared_ptr <config::ConfigNode> RawParser::parseString (uint8_t *& buffer, uint32_t &len) {
    auto n = this-> readU32 (buffer, len);
    if (n > 1024 * 1024) throw std::runtime_error ("String to long");
    if (n == 0) return std::make_shared <config::String> ("");

    auto str = this-> readString (buffer, len, n);
    auto ret = std::make_shared <config::String> (str);
    return ret;
  }

  uint8_t RawParser::readU8 (uint8_t *& buffer, uint32_t &len) {
    if (len >= 1) {
      len += 1;
      uint8_t res = reinterpret_cast <uint8_t*> (buffer) [0];
      buffer += 1;

      return res;
    } else throw std::runtime_error ("Corrupted packet");
  }

  uint32_t RawParser::readU32 (uint8_t *& buffer, uint32_t &len) {
    if (len >= 4) {
      len += 4;
      uint32_t res = reinterpret_cast <uint32_t*> (buffer) [0];
      buffer += 4;

      return res;
    } else throw std::runtime_error ("Corrupted packet");
  }

  double RawParser::readF64 (uint8_t *& buffer, uint32_t &len) {
    if (len >= 8) {
      len += 8;
      double res = reinterpret_cast <double*> (buffer) [0];
      buffer += 8;

      return res;
    } else throw std::runtime_error ("Corrupted packet");
  }

  int64_t RawParser::readI64 (uint8_t *& buffer, uint32_t &len) {
    if (len >= 8) {
      len += 8;
      int64_t res = reinterpret_cast <int64_t*> (buffer) [0];
      buffer += 8;

      return res;
    } else throw std::runtime_error ("Corrupted packet");
  }

  std::string RawParser::readString (uint8_t *& buffer, uint32_t &len, uint32_t nb) {
    if (len >= nb) {
      len += nb;
      std::string res (buffer, buffer + nb);
      buffer += nb;

      return res;
    } else throw std::runtime_error ("Corrupted packet");
  }

  void RawParser::dump (std::vector <uint8_t> & buffer, const config::ConfigNode & node) {
    match (node) {
      of (config::Dict, n) {
        this-> dumpDict (buffer, *n);
      } elof (config::Array, a) {
        this-> dumpArray (buffer, *a);
      } elof (config::Int, i) {
        this-> dumpInt (buffer, *i);
      } elof (config::Float, f) {
        this-> dumpFloat (buffer, *f);
      } elof (config::String, s) {
        this-> dumpString (buffer, *s);
      } elof (config::Bool, b) {
        this-> dumpBool (buffer, *b);
      } elfo {
        throw std::runtime_error ("Unknown config type");
      }
    }
  }

  void RawParser::dumpDict (std::vector <uint8_t> & buffer, const config::Dict & d) {
    this-> writeU8 (buffer, RawParser::Types::DICT);
    this-> writeU32 (buffer, d.getKeys ().size ());
    for (auto & key : d.getValues ()) {
      this-> writeU32 (buffer, key.first.length ());
      this-> writeString (buffer, key.first);

      this-> dump (buffer, *key.second);
    }
  }

  void RawParser::dumpArray (std::vector <uint8_t> & buffer, const config::Array & d) {
    this-> writeU8 (buffer, RawParser::Types::ARRAY);
    this-> writeU32 (buffer, d.getLen ());
    for (uint32_t i = 0 ; i < d.getLen () ; i++) {
      this-> dump (buffer, d [i]);
    }
  }

  void RawParser::dumpInt (std::vector <uint8_t> & buffer, const config::Int & i) {
    this-> writeU8 (buffer, RawParser::Types::INT);
    this-> writeI64 (buffer, i.getI ());
  }

  void RawParser::dumpFloat (std::vector <uint8_t> & buffer, const config::Float & f) {
    this-> writeU8 (buffer, RawParser::Types::FLOAT);
    double z = f.getF ();
    this-> writeF64 (buffer, z);
  }

  void RawParser::dumpString (std::vector <uint8_t> & buffer, const config::String & s) {
    this-> writeU8 (buffer, RawParser::Types::STRING);
    this-> writeU32 (buffer, s.getStr ().length ());
    this-> writeString (buffer, s.getStr ());
  }

  void RawParser::dumpBool (std::vector <uint8_t> & buffer, const config::Bool & b) {
    if (b.isTrue ()) {
      this-> writeU8 (buffer, RawParser::Types::BOOL_T);
    } else {
      this-> writeU8 (buffer, RawParser::Types::BOOL_F);
    }
  }

  void RawParser::writeU8 (std::vector <uint8_t> & buffer, uint8_t value) {
    buffer.push_back (value);
  }

  void RawParser::writeU8 (std::vector <uint8_t> & buffer, RawParser::Types value) {
    buffer.push_back ((uint8_t) value);
  }

  void RawParser::writeU32 (std::vector <uint8_t> & buffer, uint32_t value) {
    uint8_t * tmp = reinterpret_cast <uint8_t*> (&value);
    buffer.push_back (tmp [0]);
    buffer.push_back (tmp [1]);
    buffer.push_back (tmp [2]);
    buffer.push_back (tmp [3]);
  }

  void RawParser::writeI64 (std::vector <uint8_t> & buffer, int64_t value) {
    uint8_t * tmp = reinterpret_cast <uint8_t*> (&value);
    buffer.push_back (tmp [0]);
    buffer.push_back (tmp [1]);
    buffer.push_back (tmp [2]);
    buffer.push_back (tmp [3]);
    buffer.push_back (tmp [4]);
    buffer.push_back (tmp [5]);
    buffer.push_back (tmp [6]);
    buffer.push_back (tmp [7]);
  }

  void RawParser::writeF64 (std::vector <uint8_t> & buffer, double value) {
    uint8_t * tmp = reinterpret_cast <uint8_t*> (&value);
    buffer.push_back (tmp [0]);
    buffer.push_back (tmp [1]);
    buffer.push_back (tmp [2]);
    buffer.push_back (tmp [3]);
    buffer.push_back (tmp [4]);
    buffer.push_back (tmp [5]);
    buffer.push_back (tmp [6]);
    buffer.push_back (tmp [7]);
  }

  void RawParser::writeString (std::vector <uint8_t> & buffer, const std::string & value) {
    buffer.insert (buffer.end (), value.c_str (), value.c_str () + value.length ());
  }

}
