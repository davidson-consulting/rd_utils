#include "stream.hh"


namespace rd_utils::concurrency::actor {

  ActorStream::ActorStream (std::shared_ptr <net::TcpStream> input, std::shared_ptr <net::TcpStream> output, bool left) :
    _input (input)
    , _output (output)
    , _left (left)
  {}

  bool ActorStream::next () {
    int x = this-> _input-> receiveInt ();
    return x == 1 && this-> _input-> isOpen ();
  }

  void ActorStream::write (const utils::config::ConfigNode & node) {
    this-> _output-> sendInt (1);
    utils::raw::dump (*this-> _output, node);
  }

  void ActorStream::stop () {
    this-> _output-> sendInt (0);
  }

  std::shared_ptr <utils::config::ConfigNode> ActorStream::read () {
    if (this-> next ()) {
      std::cout << "Reading" << std::endl;
      return utils::raw::parse (*this-> _input);
    }

    return nullptr;
  }

  ActorStream::~ActorStream () {
    if (this-> _output != nullptr && !this-> _left) {
      std::cout << "Stoping " << std::endl;
      this-> stop ();
    }

    this-> _output = nullptr;
    this-> _input = nullptr;
  }

}
