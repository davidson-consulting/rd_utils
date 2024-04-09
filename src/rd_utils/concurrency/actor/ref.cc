
#include "sys.hh"
#include <rd_utils/utils/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include "ref.hh"


namespace rd_utils::concurrency::actor {


  ActorRef::ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, std::shared_ptr<net::TcpStream> & conn, ActorSystem * sys) :
    _isLocal (local)
    , _name (name)
    , _addr (addr)
    , _conn (conn)
    , _sys (sys)
  {}

  void ActorRef::send (const rd_utils::utils::config::ConfigNode & node) {
    if (this-> _conn == nullptr || !this-> _conn-> isOpen ()) {
      try {
        this-> _conn = std::make_shared<net::TcpStream> (this-> _addr);
        this-> _conn-> connect ();
      } catch (...) {
        throw std::runtime_error ("Failed to reconnect to remote actor system : " + this-> _addr.toString ());
      }
    }

    this-> _conn-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_MSG));
    this-> _conn-> sendInt (this-> _name.length ());
    this-> _conn-> send (this-> _name.c_str (), this-> _name.length ());
    this-> _conn-> sendInt (this-> _sys-> port ());

    utils::raw::dump (*this-> _conn, node);
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> ActorRef::request (const rd_utils::utils::config::ConfigNode & node) {
    if (this-> _conn == nullptr || !this-> _conn-> isOpen ()) {
      try {
        this-> _conn = std::make_shared<net::TcpStream> (this-> _addr);
        this-> _conn-> connect ();
      } catch (...) {
        throw std::runtime_error ("Failed to reconnect to remote actor system : " + this-> _addr.toString ());
      }
    }

    this-> _conn-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ));
    this-> _conn-> sendInt (this-> _name.length ());
    this-> _conn-> send (this-> _name.c_str (), this-> _name.length ());
    this-> _conn-> sendInt (this-> _sys-> port ());

    uint64_t uniqId = this-> _sys-> genUniqId ();
    this-> _conn-> sendInt (uniqId);
    utils::raw::dump (*this-> _conn, node);

    for (;;) {
      this-> _sys-> _waitResponse.wait ();
      ActorSystem::Response resp;
      if (this-> _sys-> _responses.receive (resp)) {
        if (resp.reqId == uniqId) {
          return resp.msg;
        }

        this-> _sys-> pushResponse (resp);
      }
    }
  }

  void ActorRef::response (uint64_t reqId, const rd_utils::utils::config::ConfigNode & node) {
    if (this-> _conn == nullptr || !this-> _conn-> isOpen ()) {
      try {
        this-> _conn = std::make_shared<net::TcpStream> (this-> _addr);
        this-> _conn-> connect ();
      } catch (...) {
        throw std::runtime_error ("Failed to reconnect to remote actor system : " + this-> _addr.toString ());
      }
    }

    this-> _conn-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP));
    this-> _conn-> sendInt (reqId);

    utils::raw::dump (*this-> _conn, node);
  }

  void ActorRef::responseBig (uint64_t reqId, std::shared_ptr <rd_utils::memory::cache::collection::CacheArrayBase> & array) {
    if (this-> _conn == nullptr || !this-> _conn-> isOpen ()) {
      try {
        this-> _conn = std::make_shared<net::TcpStream> (this-> _addr);
        this-> _conn-> connect ();
      } catch (...) {
        throw std::runtime_error ("Failed to reconnect to remote actor system : " + this-> _addr.toString ());
      }
    }

    this-> _conn-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP_BIG));
    this-> _conn-> sendInt (reqId);

    array-> send (*this-> _conn, ARRAY_BUFFER_SIZE);
  }



  const std::string & ActorRef::getName () const {
    return this-> _name;
  }

  void ActorRef::dispose () {
    this-> _conn = nullptr;
  }

  ActorRef::~ActorRef () {
    this-> dispose ();
  }

}
