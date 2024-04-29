#include "sys.hh"
#include <thread>
#include <rd_utils/utils/raw_parser.hh>


#include "ref.hh"
#include "base.hh"
#include "stream.hh"

namespace rd_utils::concurrency::actor {

  ActorSystem::ActorSystem (net::SockAddrV4 addr, int nbThreads, int maxCon) :
    _server (addr, nbThreads == -1 ? std::thread::hardware_concurrency() : nbThreads, maxCon),
    _localConn (nullptr)
  {}

  void ActorSystem::start () {
    this-> _server.start (this, &ActorSystem::onSession);
  }

  void ActorSystem::remove (const std::string & name) {
    auto act = this-> _actors.find (name);
    if (act != this-> _actors.end ()) {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        act-> second-> onQuit ();
      }

      this-> _actors.erase (name);
      this-> _actorMutexes.erase (name);
    }
  }

  std::shared_ptr <ActorRef> ActorSystem::localActor (const std::string & name) {
    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) throw std::runtime_error ("No local actor named : " + name);

    auto addr = net::SockAddrV4 (net::Ipv4Address ("0.0.0.0"), this-> _server.port ());

    if (this-> _localConn == nullptr) {
      try {
        this-> _localConn = std::make_shared<net::TcpPool> (addr, 4);
      } catch (...) {
        this-> _localConn = nullptr;
        throw std::runtime_error ("Failed to connect to local server");
      }
    }

    return std::make_shared<ActorRef> (true, name, addr, this-> _localConn, this);
  }


  std::shared_ptr<ActorRef> ActorSystem::remoteActor (const std::string & name, net::SockAddrV4 addr, bool check) {
    auto str = this-> _conn.find (addr.toString ());
    if (str == this-> _conn.end ()) {
      try {
        auto str_ = std::make_shared <net::TcpPool> (addr, 4);
        this-> _conn.emplace (addr.toString (), str_);
        str = this-> _conn.find (addr.toString ());
      } catch (...) {
        throw std::runtime_error ("Failed to connect to remote actor system : " + addr.toString ());
      }
    }

    if (check) {
      auto session = str-> second-> get ();

      session-> sendInt ((uint32_t) ActorSystem::Protocol::ACTOR_EXIST_REQ);
      session-> sendInt (name.length ());
      session-> send (name.c_str (), name.length ());

      int exists = session-> receiveInt ();
      if (exists == 0) throw std::runtime_error ("Remote actor " + name + " does not exist in remote system : " + addr.toString ());
    }

    return std::make_shared<ActorRef> (false, name, addr, str-> second, this);
  }

  uint64_t ActorSystem::genUniqId () {
    this-> _lastU += 1;
    return this-> _lastU;
  }

  void ActorSystem::dispose () {
    for (auto & it : this-> _actors) {
      WITH_LOCK (this-> _actorMutexes.find (it.first)-> second) {
        it.second-> onQuit ();
      }
    }

    this-> _actors.clear ();
    this-> _actorMutexes.clear ();
    this-> _server.stop ();
  }

  void ActorSystem::join () {
    this-> _server.join ();
  }

  uint32_t ActorSystem::port () {
    return this-> _server.port ();
  }

  ActorSystem::~ActorSystem () {
    this-> dispose ();
  }

  void ActorSystem::pushResponse (Response rep) {
    this-> _responses.send (rep);
    this-> _waitResponse.post ();
  }

  void ActorSystem::pushResponseBig (ResponseBig rep) {
    this-> _responseBigs.send (rep);
    this-> _waitResponseBig.post ();
  }

  void ActorSystem::pushResponseStream (ResponseStream rep) {
    this-> _responseStreams.send (rep);
    this-> _waitResponseStream.post ();
  }

  void ActorSystem::onSession (net::TcpSessionKind kind, std::shared_ptr <net::TcpStream> session) {
    auto protId = session-> receiveInt ();
    switch ((ActorSystem::Protocol) protId) {
    case ActorSystem::Protocol::ACTOR_EXIST_REQ:
      this-> onActorExistReq (session);
      break;
    case ActorSystem::Protocol::ACTOR_MSG:
      this-> onActorMsg (session);
      break;
    case ActorSystem::Protocol::ACTOR_REQ:
      this-> onActorReq (session);
      break;
    case ActorSystem::Protocol::ACTOR_REQ_BIG:
      this-> onActorReqBig (session);
      break;
    case ActorSystem::Protocol::ACTOR_RESP:
      this-> onActorResp (session);
      break;
    case ActorSystem::Protocol::ACTOR_RESP_BIG:
      this-> onActorRespBig (session);
      break;
    case ActorSystem::Protocol::SYSTEM_KILL_ALL:
      this-> onSystemKill (session);
      break;
    default :
      session-> close ();
      break;
    }
  }

  void ActorSystem::onActorExistReq (std::shared_ptr <net::TcpStream> session) {
    auto name = this-> readActorName (session);
    auto it = this-> _actors.find (name);
    if (it != this-> _actors.end ()) {
      session-> sendInt (1);
    } else {
      session-> sendInt (0);
    }
  }

  void ActorSystem::onActorMsg (std::shared_ptr <net::TcpStream> session) {
    auto name = this-> readActorName (session);
    this-> readAddress (session);
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      session-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      session-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          it-> second-> onMessage (*msg);
        } catch (...) {

        }
      }
    }
  }

  void ActorSystem::onActorReq (std::shared_ptr <net::TcpStream> session) {
    concurrency::timer t;
    auto name = this-> readActorName (session);
    auto addr = this-> readAddress (session);
    auto reqId = session-> receiveInt ();
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      session-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      session-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          t.reset ();
          auto result = it-> second-> onRequest (*msg);
          auto actorRef = this-> remoteActor ("", addr, false);
          actorRef-> response (reqId, result);
        } catch (std::runtime_error & e) {
          session-> close ();
        }
      }
    }
  }

  void ActorSystem::onActorReqBig (std::shared_ptr <net::TcpStream> session) {
    concurrency::timer t;
    auto name = this-> readActorName (session);
    auto addr = this-> readAddress (session);
    auto reqId = session-> receiveInt ();
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      session-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      session-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          t.reset ();
          auto result = it-> second-> onRequestList (*msg);
          if (result != nullptr) {
            auto actorRef = this-> remoteActor ("", addr, false);
            actorRef-> responseBig (reqId, result);
          }
        } catch (std::runtime_error & e) {
          session-> close ();
        }
      }
    }
  }

  void ActorSystem::onActorResp (std::shared_ptr <net::TcpStream> session) {
    auto reqId = session-> receiveInt ();
    auto hasValue = session-> receiveInt ();
    if (hasValue == 1) {
      auto value = this-> readMessage (session);
      this-> pushResponse ({.reqId = reqId, .msg = value});
    } else {
      this-> pushResponse ({.reqId = reqId, .msg = nullptr});
    }
  }

  void ActorSystem::onActorRespBig (std::shared_ptr <net::TcpStream> session) {
    auto reqId = session-> receiveInt ();
    auto hasValue = session-> receiveInt ();
    if (hasValue == 1) {
      auto array = std::make_shared <rd_utils::memory::cache::collection::CacheArrayBase> ();
      array-> recv (*session, ARRAY_BUFFER_SIZE);

      this-> pushResponseBig ({.reqId = reqId, .msg = array});
    } else {
      this-> pushResponseBig ({.reqId = reqId, .msg = nullptr});
    }
  }

  void ActorSystem::onSystemKill (std::shared_ptr<net::TcpStream> session) {
    this-> dispose ();
  }

  std::string ActorSystem::readActorName (std::shared_ptr <net::TcpStream> stream) {
    uint32_t len = stream-> receiveInt ();
    if (len <= 32) {
      char * str = new char [len + 1];
      stream-> receive (str, len);
      str [len] = '\0';
      std::string ret (str);
      delete [] str;

      return ret;
    } else return "";
  }

  net::SockAddrV4 ActorSystem::readAddress (std::shared_ptr <net::TcpStream> stream) {
    uint32_t port = stream-> receiveInt ();
    auto ip = stream-> addr ().ip ();

    return net::SockAddrV4 (ip, port);
  }

  std::shared_ptr<utils::config::ConfigNode> ActorSystem::readMessage (std::shared_ptr <net::TcpStream> stream) {
    try {
      return utils::raw::parse (*stream);
    } catch (...) {
      return nullptr;
    }
  }

}
