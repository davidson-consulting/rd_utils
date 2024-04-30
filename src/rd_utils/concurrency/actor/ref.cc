
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

    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_MSG));
    session-> sendU32 (this-> _name.length ());
    session-> send (this-> _name.c_str (), this-> _name.length ());
    session-> sendU32 (this-> _sys-> port ());

    utils::raw::dump (*session, node);
  }

  std::shared_ptr<rd_utils::utils::config::ConfigNode> ActorRef::request (const rd_utils::utils::config::ConfigNode & node) {
    concurrency::timer t;
    auto session = this-> _conn-> get ();

    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_REQ));
    session-> sendU32 (this-> _name.length ());
    session-> send (this-> _name.c_str (), this-> _name.length ());
    session-> sendU32 (this-> _sys-> port ());

    uint64_t uniqId = this-> _sys-> genUniqId ();
    session-> sendU64 (uniqId);

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

  std::shared_ptr <ActorStream> ActorRef::requestStream (const rd_utils::utils::config::ConfigNode & msg) {
    concurrency::timer t;
    auto session = this-> _conn-> get ();

    session-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_REQ_STREAM);
    session-> sendU32 (this-> _name.length ());
    session-> send (this-> _name.c_str (), this-> _name.length ());
    session-> sendU32 (this-> _sys-> port ());

    uint64_t uniqId = this-> _sys-> genUniqId ();
    session-> sendU64 (uniqId);
    utils::raw::dump (*session, msg);

    for (;;) {
      t.reset ();
      this-> _sys-> _waitResponseStream.wait ();
      ActorSystem::ResponseStream resp;
      if (this-> _sys-> _responseStreams.receive (resp)) {
        if (resp.reqId == uniqId) {
          return std::make_shared <ActorStream> (std::move (*resp.stream), std::move (session), true);
        }

        this-> _sys-> pushResponseStream (resp);
      }
    }
  }


  void ActorRef::response (uint64_t reqId, std::shared_ptr <rd_utils::utils::config::ConfigNode> node) {
    auto session = this-> _conn-> get ();
    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP));
    session-> sendU64 (reqId);
    if (node == nullptr) {
      session-> sendU32 (0);
    } else {
      session-> sendU32 (1);
      utils::raw::dump (*session, *node);
    }
  }

  void ActorRef::responseBig (uint64_t reqId, std::shared_ptr <rd_utils::memory::cache::collection::ArrayListBase> & array) {
    auto session = this-> _conn-> get ();

    session-> sendU32 ((uint32_t) (ActorSystem::Protocol::ACTOR_RESP_BIG));
    session-> sendU64 (reqId);

    if (array == nullptr) {
      session-> sendU32 (0);
    } else {
      session-> sendU32 (1);
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
    if (!this-> _isLocal) {
      this-> _sys-> closingRemote (this-> _addr);
    }

    this-> _conn = nullptr;
  }

  ActorRef::~ActorRef () {
    this-> dispose ();
  }

}
