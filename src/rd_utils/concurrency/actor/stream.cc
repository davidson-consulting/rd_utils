#include "stream.hh"
#include <rd_utils/utils/config/dict.hh>


namespace rd_utils::concurrency::actor {

  ActorStream::ActorStream (net::TcpSession && input, net::TcpSession && output, bool left) :
    _input (std::move (input))
    , _output (std::move (output))
    , _left (left)
  {}

  void ActorStream::write (const utils::config::ConfigNode & node) {
    utils::raw::dump (*this-> _output, node);
  }

  std::shared_ptr <utils::config::ConfigNode> ActorStream::read () {
    try {
      return utils::raw::parse (*this-> _input);
    } catch (...) {
      return std::make_shared <utils::config::Dict> ();
    }

    return std::make_shared <utils::config::Dict> ();
  }

  void ActorStream::writeStr (const std::string & msg) {
    this-> _output-> sendU32 (msg.length ());
    if (msg.length () != 0) {
      this-> _output-> send (msg.c_str (), msg.length ());
    }
  }

  std::string ActorStream::readStr () {
    auto n = this-> _input-> receiveU32 ();
    if (n != 0) {
      if (n > 1024 * 1024) throw std::runtime_error ("String to long");

      char * buffer = new char[n + 1];
      this-> _input-> receive (buffer, n);
      buffer [n] = 0;

      auto ret = std::string (buffer);
      delete [] buffer;

      return ret;
    }

    return "";
  }

  void ActorStream::writeU8 (uint8_t i) {
    this-> _output-> sendChar (i);
  }

  void ActorStream::writeU32 (uint32_t i) {
    this-> _output-> sendU32 (i);
  }

  void ActorStream::writeU64 (uint64_t i) {
    this-> _output-> sendU64 (i);
  }

  uint8_t ActorStream::readOr (uint8_t v) {
    if (this-> isOpen ()) {
      auto res = this-> readU8 ();
      if (this-> isOpen ()) return res;
    }

    return v;
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

  void ActorStream::close () {
    this-> _input-> close ();
    this-> _output-> close ();
  }

  ActorStream::~ActorStream () {
    this-> close ();
    this-> _input.dispose ();
    this-> _output.dispose ();
  }

}
