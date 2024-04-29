
#include "sys.hh"
#include <rd_utils/utils/_.hh>
#include <rd_utils/utils/raw_parser.hh>
#include "ref.hh"


namespace rd_utils::concurrency::actor {


  ActorRef::ActorRef (bool local, const std::string & name, net::SockAddrV4 addr, std::shared_ptr<net::TcpPool> & conn, ActorSystem * sys) :
    _isLocal (local)
    , _name (name)
    , _addr (addr)
    , _conn (conn)
    , _sys (sys)
  {}

  void ActorRef::send (const rd_utils::utils::config::ConfigNode & node) {
    auto session = this-> _conn-> get ();

    session-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_MSG));
    session-> sendInt (this-> _name.length ());
    session-> send (this-> _name.c_str (), this-> _name.length ());
    session-> sendInt (this-> _sys-> port ());

    utils::raw::dump (*session, node);
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> ActorRef::request (const rd_utils::utils::config::ConfigNode & node) {
    auto session = this-> _conn-> get ();

    session-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ));
    session-> sendInt (this-> _name.length ());
    session-> send (this-> _name.c_str (), this-> _name.length ());
    session-> sendInt (this-> _sys-> port ());

    uint64_t uniqId = this-> _sys-> genUniqId ();
    session-> sendInt (uniqId);
    utils::raw::dump (*session, node);

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

  void ActorRef::response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> node) {
    auto session = this-> _conn-> get ();
    session-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP));
    session-> sendInt (reqId);
    if (node == nullptr) {
      session-> sendInt (0);
    } else {
      session-> sendInt (1);
      utils::raw::dump (*session, *node);
    }
  }

  void ActorRef::responseBig (uint64_t reqId, std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> & array) {
    auto session = this-> _conn-> get ();

    session-> sendInt ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP_BIG));
    session-> sendInt (reqId);

    if (array == nullptr) {
      session-> sendInt (0);
    } else {
      session-> sendInt (1);
      array-> send (*session, ARRAY_BUFFER_SIZE);
    }
  }

  std::shared_ptr <net::TcpPool> ActorRef::getSession () {
    return this-> _conn;
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
