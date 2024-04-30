
#ifndef __PROJECT__
#define __PROJECT__ "ACTOR_SYSTEM"
#endif

#include "sys.hh"
#include <thread>
#include <rd_utils/utils/raw_parser.hh>


#include "ref.hh"
#include "base.hh"
#include "stream.hh"

namespace rd_utils::concurrency::actor {

  ActorSystem::ActorSystem (net::SockAddrV4 addr, int nbThreads, int maxCon) :
    _server (addr, nbThreads == -1 ? std::thread::hardware_concurrency() : nbThreads, maxCon)
    ,_nbThreads (nbThreads == -1 ? std::thread::hardware_concurrency() : nbThreads)
    ,_localConn (nullptr)
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

  std::shared_ptr <ActorRef> ActorSystem::localActor (const std::string & name, bool check) {
    if (check) {
      auto it = this-> _actors.find (name);
      if (it == this-> _actors.end ()) throw std::runtime_error ("No local actor named : " + name);
    }

    auto addr = net::SockAddrV4 (net::Ipv4Address ("127.0.0.1"), this-> _server.port ());
    if (this-> _localConn == nullptr) {
      try {
        this-> _localConn = std::make_shared<net::TcpPool> (addr, this-> _nbThreads * 2);
      } catch (...) {
        this-> _localConn = nullptr;
        std::cout << "Here ?" << addr << " " << std::endl;
        throw std::runtime_error ("Failed to connect to local server");
      }
    }

    return std::make_shared<ActorRef> (true, name, addr, this-> _localConn, this);
  }

  std::shared_ptr<ActorRef> ActorSystem::remoteActor (const std::string & name, net::SockAddrV4 addr, bool check) {
    auto isl = this-> isLocal (addr);
    if (isl) return this-> localActor (name, check);

    std::shared_ptr <net::TcpPool> pool = nullptr;
    WITH_LOCK (this-> _connM) {
      auto str = this-> _conn.find (addr.toString ());
      if (str == this-> _conn.end ()) {
        try {
          auto str_ = std::make_shared <net::TcpPool> (addr, this-> _nbThreads * 2);
          this-> _conn.emplace (addr.toString (), std::make_pair<uint32_t, std::shared_ptr <net::TcpPool> > (0, std::move (str_)));
          str = this-> _conn.find (addr.toString ());
        } catch (...) {
          throw std::runtime_error ("Failed to connect to remote actor system : " + addr.toString ());
        }
      }

      str-> second.first += 1;
      pool = str-> second.second;
    }

    if (check) {
      auto session = pool-> get ();
      session-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_EXIST_REQ);
      session-> sendU32 (name.length ());
      session-> send (name.c_str (), name.length ());

      int exists = session-> receiveU32 ();
      if (exists == 0) throw std::runtime_error ("Remote actor " + name + " does not exist in remote system : " + addr.toString ());
    }

    return std::make_shared<ActorRef> (false, name, addr, pool, this);
  }

  uint64_t ActorSystem::genUniqId () {
    this-> _lastU += 1;
    return this-> _lastU;
  }

  void ActorSystem::closingRemote (net::SockAddrV4 addr) {
    WITH_LOCK (this-> _connM) {
      auto str = this-> _conn.find (addr.toString ());
      if (str != this-> _conn.end ()) {
        str-> second.first -= 1;
        if (str-> second.first == 0) {
          this-> _conn.erase (addr.toString ());
        }
      }
    }
  }

  void ActorSystem::dispose () {
    for (auto & it : this-> _actors) {
      WITH_LOCK (this-> _actorMutexes.find (it.first)-> second) {
        it.second-> onQuit ();
      }
    }

    this-> _conn.clear ();
    this-> _actors.clear ();
    this-> _actorMutexes.clear ();
    this-> _server.stop ();
  }

  bool ActorSystem::isLocal (net::SockAddrV4 addr) const {
    if (this-> _server.port () != addr.port ()) return false;

    auto locals = net::Ipv4Address::getAllIps ();
    for (auto & it : locals) {
      if (it.toN () == addr.ip ().toN ()) {
        return true;
      }
    }

    return false;
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

  void ActorSystem::onSession (net::TcpSessionKind kind, std::shared_ptr <net::TcpSession> session) {
    auto protId = (*session)-> receiveU32 ();
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
    case ActorSystem::Protocol::ACTOR_REQ_STREAM:
      this-> onActorReqStream (session);
      break;
    case ActorSystem::Protocol::ACTOR_RESP:
      this-> onActorResp (session);
      break;
    case ActorSystem::Protocol::ACTOR_RESP_BIG:
      this-> onActorRespBig (session);
      break;
    case ActorSystem::Protocol::ACTOR_RESP_STREAM:
      this-> onActorRespStream (session);
      break;
    case ActorSystem::Protocol::SYSTEM_KILL_ALL:
      this-> onSystemKill (session);
      break;
    default :
      (*session)-> close ();
      break;
    }
  }

  void ActorSystem::onActorExistReq (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);
    LOG_DEBUG ("Actor existence : ", name);

    auto it = this-> _actors.find (name);
    if (it != this-> _actors.end ()) {
      (*session)-> sendU32 (1);
    } else {
      (*session)-> sendU32 (0);
    }
  }

  void ActorSystem::onActorMsg (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);
    LOG_DEBUG ("Actor msg : ", name);

    this-> readAddress (session);
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      (*session)-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      (*session)-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          it-> second-> onMessage (*msg);
        } catch (...) {

        }
      }
    }
  }

  void ActorSystem::onActorReq (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);

    auto addr = this-> readAddress (session);
    auto reqId = (*session)-> receiveU64 ();
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      (*session)-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      (*session)-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          auto result = it-> second-> onRequest (*msg);
          auto actorRef = this-> remoteActor ("", addr, false);
          actorRef-> response (reqId, result);
        } catch (std::runtime_error & e) {
          (*session)-> close ();
        }
      }
    }
  }

  void ActorSystem::onActorReqBig (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);

    auto addr = this-> readAddress (session);
    auto reqId = (*session)-> receiveU64 ();
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      (*session)-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      (*session)-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          auto result = it-> second-> onRequestList (*msg);
          if (result != nullptr) {
            auto actorRef = this-> remoteActor ("", addr, false);
            actorRef-> responseBig (reqId, result);
          }
        } catch (std::runtime_error & e) {
          (*session)-> close ();
        }
      }
    }
  }

  void ActorSystem::onActorReqStream (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);

    auto addr = this-> readAddress (session);
    auto reqId = (*session)-> receiveU64 ();
    auto msg = this-> readMessage (session);

    if (msg == nullptr) {
      (*session)-> close ();
      return;
    }

    auto it = this-> _actors.find (name);
    if (it == this-> _actors.end ()) {
      (*session)-> close ();
    } else {
      WITH_LOCK (this-> _actorMutexes.find (name)-> second) {
        try {
          auto actorRef = this-> remoteActor ("", addr, false);
          auto conn = actorRef-> getSession ()-> get ();

          conn-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_RESP_STREAM);
          conn-> sendU32 (reqId);

          ActorStream stream (std::move (*session), std::move (conn), false);

          it-> second-> onStream (*msg, stream);
        } catch (std::runtime_error & e) {
          (*session)-> close ();
        }
      }
    }
  }

  void ActorSystem::onActorResp (std::shared_ptr <net::TcpSession> session) {
    auto reqId = (*session)-> receiveU64 ();
    LOG_DEBUG ("Actor resp : ", reqId);

    auto hasValue = (*session)-> receiveU32 ();
    if (hasValue == 1) {
      auto value = this-> readMessage (session);
      this-> pushResponse ({.reqId = reqId, .msg = value});
    } else {
      this-> pushResponse ({.reqId = reqId, .msg = nullptr});
    }
  }

  void ActorSystem::onActorRespBig (std::shared_ptr <net::TcpSession> session) {
    auto reqId = (*session)-> receiveU64 ();

    auto hasValue = (*session)-> receiveU32 ();
    if (hasValue == 1) {
      auto array = std::make_shared <rd_utils::memory::cache::collection::CacheArrayBase> ();
      array-> recv (*(*session), ARRAY_BUFFER_SIZE);

      this-> pushResponseBig ({.reqId = reqId, .msg = array});
    } else {
      this-> pushResponseBig ({.reqId = reqId, .msg = nullptr});
    }
  }

  void ActorSystem::onActorRespStream (std::shared_ptr<net::TcpSession> session) {
    auto req = (*session)-> receiveU64 ();
    this-> pushResponseStream ({.reqId = req, .stream = session});
  }

  void ActorSystem::onSystemKill (std::shared_ptr<net::TcpSession> session) {
    this-> dispose ();
  }

  std::string ActorSystem::readActorName (std::shared_ptr <net::TcpSession> stream) {
    uint32_t len = (*stream)-> receiveU32 ();
    if (len <= 32) {
      char * str = new char [len + 1];
      (*stream)-> receive (str, len);
      str [len] = '\0';
      std::string ret (str);
      delete [] str;

      return ret;
    } else return "";
  }

  net::SockAddrV4 ActorSystem::readAddress (std::shared_ptr <net::TcpSession> stream) {
    uint32_t port = (*stream)-> receiveU32 ();
    auto ip = (*stream)-> addr ().ip ();

    return net::SockAddrV4 (ip, port);
  }

  std::shared_ptr<utils::config::ConfigNode> ActorSystem::readMessage (std::shared_ptr <net::TcpSession> stream) {
    try {
      return utils::raw::parse (*(*stream));
    } catch (...) {
      return nullptr;
    }
  }

}
