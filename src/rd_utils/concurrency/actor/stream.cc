#include "stream.hh"
#include <rd_utils/utils/config/dict.hh>


namespace rd_utils::concurrency::actor {

  ActorStream::ActorStream (std::shared_ptr <net::TcpStream> input, std::shared_ptr <net::TcpStream> output)
    : _input (input)
    , _output (output)
  {}

  void ActorStream::writeStr (const std::string & msg) {
    this-> _output-> sendU32 (msg.length (), true);
    if (msg.length () != 0) {
      this-> _output-> sendStr (msg, true);
    }
  }

  std::string ActorStream::readStr () {
    auto len = this-> _input-> receiveU32 ();
    if (len != 0) {
      if (len > 1024 * 1024) throw std::runtime_error ("String to long");

      return this-> _input-> receiveStr (len);
    }

    return "";
  }

  void ActorStream::writeU8 (uint8_t i) {
    this-> _output-> sendChar (i, true);
  }

  void ActorStream::writeU32 (uint32_t i) {
    this-> _output-> sendU32 (i, true);
  }

  void ActorStream::writeU64 (uint64_t i) {
    this-> _output-> sendU64 (i, true);
  }

  uint8_t ActorStream::readOr (uint8_t v) {
    uint8_t i;
    if (this-> _input-> receiveChar (i)) return i;
    else return v;
  }

  uint8_t ActorStream::readU8 () {
    return this-> _input-> receiveChar ();
  }

  uint32_t ActorStream::readU32 () {
    return this-> _input-> receiveU32 ();
  }

  uint64_t ActorStream::readU64 () {
    return this-> _input-> receiveU64 ();
  }

  bool ActorStream::isOpen () {
    return this-> _input-> isOpen () && this-> _output-> isOpen ();
  }

  ActorStream::~ActorStream () {}

}
