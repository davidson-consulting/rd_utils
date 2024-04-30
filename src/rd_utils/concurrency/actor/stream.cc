#include "stream.hh"


namespace rd_utils::concurrency::actor {

  ActorStream::ActorStream (net::TcpSession && input, net::TcpSession && output, bool left) :
    _input (std::move (input))
    , _output (std::move (output))
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
    this-> _input.dispose ();
    this-> _output.dispose ();
  }

}
