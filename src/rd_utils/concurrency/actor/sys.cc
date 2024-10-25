#define LOG_LEVEL 10

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
    _server (addr, nbThreads == -1 ? std::thread::hardware_concurrency() : nbThreads + 1, maxCon)
    ,_nbThreads (nbThreads == -1 ? std::thread::hardware_concurrency() : nbThreads + 1)
  {}

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SETTERS          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  ActorSystem& ActorSystem::joinOnEmpty (bool join) {
    this-> _stopOnEmpty = join;
    return *this;
  }

  void ActorSystem::removeActor (const std::string & name) {
    WITH_LOCK (this-> _actMut) {
      this-> _actors.erase (name);
      this-> _actorMutexes.erase (name);
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          GETTERS          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::shared_ptr <ActorRef> ActorSystem::localActor (const std::string & name, bool check) {
    if (check) {
      std::shared_ptr <ActorBase> act;
      concurrency::mutex m;
      if (!this-> getActor (name, act, m)) throw std::runtime_error ("No local actor named : " + name);
    }

    auto addr = net::SockAddrV4 (net::Ipv4Address ("127.0.0.1"), this-> _server.port ());
    auto pool = std::make_shared<net::TcpPool> (addr, 1);
    pool-> setSendTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));
    pool-> setRecvTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));

    return std::make_shared<ActorRef> (true, name, addr, pool, this);
  }

  std::shared_ptr<ActorRef> ActorSystem::remoteActor (const std::string & name, net::SockAddrV4 addr, bool check) {
    auto isl = this-> isLocal (addr);
    if (isl) return this-> localActor (name, check);

    std::shared_ptr <net::TcpPool> pool = std::make_shared <net::TcpPool> (addr, 1);
    pool-> setSendTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));
    pool-> setRecvTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));

    if (check) {
      auto session = pool-> get (5);
      session-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_EXIST_REQ, true);
      session-> sendU32 (name.length (), true);
      session-> sendStr (name, true);

      auto exists = session-> receiveU32 ();
      if (exists == 0) throw std::runtime_error ("Remote actor " + name + " does not exist in remote system : " + addr.toString ());
    }

    return std::make_shared<ActorRef> (false, name, addr, pool, this);
  }

  uint32_t ActorSystem::port () {
    return this-> _server.port ();
  }

  bool ActorSystem::getActor (const std::string & name, std::shared_ptr <ActorBase>& act, concurrency::mutex & actMut) {
    WITH_LOCK (this-> _actMut) {
      auto it = this-> _actors.find (name);
      if (it == this-> _actors.end ()) return false;

      act = it-> second;
      actMut = this-> _actorMutexes.find (name)-> second;
      return true;
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          RUNNING          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::start () {
    this-> _server.setSendTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));
    this-> _server.setRecvTimeout (static_cast <float> (ActorSystemLimits::BASE_TIMEOUT));

    this-> _server.start (this, &ActorSystem::onSession);
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ======================================          EXIT          ======================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::remove (const std::string & name, bool lock) {
    std::shared_ptr <ActorBase> act;
    concurrency::mutex actMut;
    if (this-> getActor (name, act, actMut)) {
      this-> removeActor (name);

      try {
        if (lock) {
          WITH_LOCK (actMut) {
            act-> onQuit ();
          }
        } else {
          act-> onQuit ();
        }
      } catch (...) {
        LOG_ERROR ("Actor ", name, " crashed on exit. Continuing..");
      }
    }

    if (this-> _stopOnEmpty && this-> _actors.size () == 0) {
      this-> poisonPill ();
    }
  }

  void ActorSystem::poisonPill () {
    try {
      auto local = this-> localActor ("", false);
      local-> getSession ()-> get (5)-> sendU32 ((uint32_t) ActorSystem::Protocol::SYSTEM_KILL_ALL);
    } catch (...) {
      LOG_ERROR ("Failed to connect to local system... Aborting.");
      this-> onSystemKill ();
    }
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          REQUESTS          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  uint64_t ActorSystem::genUniqId () {
    this-> _lastU += 1;
    return this-> _lastU;
  }

  void ActorSystem::registerRequestId (uint64_t id, std::shared_ptr <semaphore> who) {
    WITH_LOCK (this-> _rqMut) {
      this-> _requestIds.emplace (id, who);
    }
  }

  void ActorSystem::removeRequestId (uint64_t id) {
    WITH_LOCK (this-> _rqMut) {
      this-> _requestIds.erase (id);
    }
  }

  void ActorSystem::pushResponse (Response rep) {
    WITH_LOCK (this-> _rqMut) {
      auto it = this-> _requestIds.find (rep.reqId);
      if (it == this-> _requestIds.end ()) {
        LOG_WARN ("Request response after timeout : ", rep.reqId);
        return;
      }

      this-> _responses.emplace (rep.reqId, rep);
      it-> second-> post ();
      this-> _requestIds.erase (rep.reqId);
    }
  }

  bool ActorSystem::consumeResponse (uint64_t id, Response & resp) {
    WITH_LOCK (this-> _rqMut) {
      auto fnd = this-> _responses.find (id);
      if (fnd != this-> _responses.end ()) {
        resp = fnd-> second;
        this-> _responses.erase (id);

        return true;
      }
    }

    return false;
  }

  void ActorSystem::pushResponseBig (ResponseBig rep) {
    WITH_LOCK (this-> _rqMut) {
      auto it = this-> _requestIds.find (rep.reqId);
      if (it == this-> _requestIds.end ()) {
        LOG_WARN ("Request response after timeout");
        return;
      }

      this-> _responseBigs.emplace (rep.reqId, rep);
      it-> second-> post ();
      this-> _requestIds.erase (rep.reqId);
    }
  }

  bool ActorSystem::consumeResponseBig (uint64_t id, ResponseBig & resp) {
    WITH_LOCK (this-> _rqMut) {
      auto fnd = this-> _responseBigs.find (id);
      if (fnd != this-> _responseBigs.end ()) {
        resp = fnd-> second;
        this-> _responseBigs.erase (id);

        return true;
      }
    }

    return false;
  }

  void ActorSystem::pushResponseStream (ResponseStream rep) {
    WITH_LOCK (this-> _rqMut) {
      auto it = this-> _requestIds.find (rep.reqId);
      if (it == this-> _requestIds.end ()) {
        LOG_WARN ("Request response after timeout");
        return;
      }

      this-> _responseStreams.emplace (rep.reqId, rep);
      it-> second-> post ();
      this-> _requestIds.erase (rep.reqId);
    }
  }

  bool ActorSystem::consumeResponseStream (uint64_t id, ResponseStream & resp) {
    WITH_LOCK (this-> _rqMut) {
      auto fnd = this-> _responseStreams.find (id);
      if (fnd != this-> _responseStreams.end ()) {
        resp = fnd-> second;
        this-> _responseStreams.erase (id);

        return true;
      }
    }

    return false;
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ===================================          DISPOSING          ====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::dispose () {
    WITH_LOCK (this-> _actMut) {
      for (auto & it : this-> _actors) {
        WITH_LOCK (this-> _actorMutexes.find (it.first)-> second) {
          try {
            it.second-> onQuit ();
          } catch (...) {
            LOG_ERROR ("Actor ", it.first, " crashed on exit. Continuing..");
          }
        }
      }
    }

    this-> _actors.clear ();
    this-> _actorMutexes.clear ();
    this-> _server.stop ();
    this-> _server.dispose ();
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

  ActorSystem::~ActorSystem () {
    this-> dispose ();
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          SESSION          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  void ActorSystem::onSession (net::TcpSessionKind kind, std::shared_ptr <net::TcpSession> session) {
    try {
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
        this-> onSystemKill ();
        break;
      default :
        (*session)-> close ();
        break;
      }
    } catch (...) {
      LOG_INFO ("Session failed. Remote connection out.");
      (*session)-> close ();
    }

  }

  void ActorSystem::onActorExistReq (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);
    LOG_DEBUG ("Actor existence : ", name);

    std::shared_ptr <ActorBase> act;
    concurrency::mutex m;
    if (this-> getActor (name, act, m)) {
      (*session)-> sendU32 (1, true);
    } else {
      (*session)-> sendU32 (0, true);
    }
  }

  void ActorSystem::onActorMsg (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);
    this-> readAddress (session);
    auto msg = this-> readMessage (session);

    std::shared_ptr <ActorBase> act;
    concurrency::mutex m;
    if (!this-> getActor (name, act, m)) {
      (*session)-> close ();
      return;
    }

    WITH_LOCK (m) {
      try {
        act-> onMessage (*msg);
      } catch (...) {
        (*session)-> close ();
      }
    }
  }

  void ActorSystem::onActorReq (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);

    auto addr = this-> readAddress (session);
    auto reqId = (*session)-> receiveU64 ();
    auto msg = this-> readMessage (session);

    std::shared_ptr <ActorBase> act;
    concurrency::mutex m;
    if (!this-> getActor (name, act, m)) {
      (*session)-> close ();
      return;
    }

    WITH_LOCK (m) {
      try {
        auto result = act-> onRequest (*msg);
        try {
          auto actorRef = this-> remoteActor ("", addr, false);
          actorRef-> response (reqId, result, 5);
        } catch (...) {
          LOG_ERROR ("Failed to send response");
          (*session)-> close ();
        }
      } catch (...) {
        LOG_ERROR ("Sesssion failed");
        (*session)-> close ();
      }
    }
  }

  void ActorSystem::onActorReqBig (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);

    auto addr = this-> readAddress (session);
    auto reqId = (*session)-> receiveU64 ();
    auto msg = this-> readMessage (session);

    std::shared_ptr <ActorBase> act;
    concurrency::mutex m;
    if (!this-> getActor (name, act, m)) {
      (*session)-> close ();
      return;
    }

    WITH_LOCK (m) {
      try {
        auto result = act-> onRequestList (*msg);
        try {
          auto actorRef = this-> remoteActor ("", addr, false);
          actorRef-> responseBig (reqId, result, 10);
        } catch (...) {
            LOG_ERROR ("Failed to send response");
            (*session)-> close ();
        }
      } catch (...) {
        LOG_ERROR ("Sesssion failed");
        (*session)-> close ();
      }
    }
  }

  void ActorSystem::onActorReqStream (std::shared_ptr <net::TcpSession> session) {
    auto name = this-> readActorName (session);

    auto addr = this-> readAddress (session);
    auto reqId = (*session)-> receiveU64 ();
    auto msg = this-> readMessage (session);

    std::shared_ptr <ActorBase> act;
    concurrency::mutex m;
    if (!this-> getActor (name, act, m)) {
      (*session)-> close ();
      return;
    }

    WITH_LOCK (m) {
      try {
        auto actorRef = this-> remoteActor ("", addr, false);
        auto conn = actorRef-> getSession ()-> get (5);

        conn-> sendU32 ((uint32_t) ActorSystem::Protocol::ACTOR_RESP_STREAM, true);
        conn-> sendU32 (reqId, true);

        ActorStream stream (std::move (*session), std::move (conn), false);

        act-> onStream (*msg, stream);
      } catch (...) {
        LOG_ERROR ("Failed to send response");
        (*session)-> close ();
      }
    }
  }

  void ActorSystem::onActorResp (std::shared_ptr <net::TcpSession> session) {
    auto reqId = (*session)-> receiveU64 ();
    auto hasValue = (*session)-> receiveU32 ();

    if (hasValue == 1) {
      std::shared_ptr<utils::config::ConfigNode> msg = nullptr;
      try {
        msg = this-> readMessage (session);
      } catch (std::runtime_error & e) {
        LOG_ERROR ("Failed to read request response message : ", e.what ());
      }

      this-> pushResponse ({.reqId = reqId, .msg = msg});
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
    auto reqId = (*session)-> receiveU64 ();
    this-> pushResponseStream ({.reqId = reqId, .stream = session});
  }

  void ActorSystem::onSystemKill () {
    WITH_LOCK (this-> _actMut) {
      for (auto & it : this-> _actors) {
        WITH_LOCK (this-> _actorMutexes.find (it.first)-> second) {
          try {
            it.second-> onQuit ();
          } catch (...) {
            LOG_ERROR ("Actor ", it.first, " crashed on exit. Continuing..");
          }
        }
      }
    }

    this-> _actors.clear ();
    this-> _actorMutexes.clear ();
    this-> _server.stop ();

    // we don't dispose the server we might be in a thread, and therefore server is waiting for this function to quit
  }

  /*!
   * ====================================================================================================
   * ====================================================================================================
   * ====================================          PRIVATE          =====================================
   * ====================================================================================================
   * ====================================================================================================
   */

  std::string ActorSystem::readActorName (std::shared_ptr <net::TcpSession> stream) {
    auto len = (*stream)-> receiveU32 ();
    if (len <= 32) {
      return (*stream)-> receiveStr (len);
    } throw std::runtime_error ("Name too long");
  }

  net::SockAddrV4 ActorSystem::readAddress (std::shared_ptr <net::TcpSession> stream) {
    auto port = (*stream)-> receiveU32 ();
    auto ip = (*stream)-> addr ().ip ();

    return net::SockAddrV4 (ip, port);
  }

  std::shared_ptr<utils::config::ConfigNode> ActorSystem::readMessage (std::shared_ptr <net::TcpSession> stream) {
    return utils::raw::parse (*(*stream));
  }

}
